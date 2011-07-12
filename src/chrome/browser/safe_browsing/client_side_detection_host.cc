// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/client_side_detection_host.h"

#include <vector>

#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/metrics/histogram.h"
#include "base/task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/browser_feature_extractor.h"
#include "chrome/browser/safe_browsing/client_side_detection_service.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/safe_browsing/csd.pb.h"
#include "chrome/common/safe_browsing/safebrowsing_messages.h"
#include "content/browser/browser_thread.h"
#include "content/browser/renderer_host/render_process_host.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/renderer_host/render_view_host_delegate.h"
#include "content/browser/renderer_host/resource_dispatcher_host.h"
#include "content/browser/renderer_host/resource_request_details.h"
#include "content/browser/tab_contents/navigation_details.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/common/content_notification_types.h"
#include "content/common/notification_service.h"
#include "content/common/view_messages.h"
#include "googleurl/src/gurl.h"

namespace safe_browsing {

// This class is instantiated each time a new toplevel URL loads, and
// asynchronously checks whether the phishing classifier should run for this
// URL.  If so, it notifies the renderer with a StartPhishingDetection IPC.
// Objects of this class are ref-counted and will be destroyed once nobody
// uses it anymore.  If |tab_contents|, |csd_service| or |host| go away you need
// to call Cancel().  We keep the |sb_service| alive in a ref pointer for as
// long as it takes.
class ClientSideDetectionHost::ShouldClassifyUrlRequest
    : public base::RefCountedThreadSafe<
          ClientSideDetectionHost::ShouldClassifyUrlRequest> {
 public:
  ShouldClassifyUrlRequest(const ViewHostMsg_FrameNavigate_Params& params,
                           TabContents* tab_contents,
                           ClientSideDetectionService* csd_service,
                           SafeBrowsingService* sb_service,
                           ClientSideDetectionHost* host)
      : canceled_(false),
        params_(params),
        tab_contents_(tab_contents),
        csd_service_(csd_service),
        sb_service_(sb_service),
        host_(host) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    DCHECK(tab_contents_);
    DCHECK(csd_service_);
    DCHECK(sb_service_);
    DCHECK(host_);
  }

  void Start() {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

    // We start by doing some simple checks that can run on the UI thread.
    UMA_HISTOGRAM_COUNTS("SBClientPhishing.ClassificationStart", 1);

    // Only classify [X]HTML documents.
    if (params_.contents_mime_type != "text/html" &&
        params_.contents_mime_type != "application/xhtml+xml") {
      VLOG(1) << "Skipping phishing classification for URL: " << params_.url
              << " because it has an unsupported MIME type: "
              << params_.contents_mime_type;
      UMA_HISTOGRAM_ENUMERATION("SBClientPhishing.PreClassificationCheckFail",
                                NO_CLASSIFY_UNSUPPORTED_MIME_TYPE,
                                NO_CLASSIFY_MAX);
      return;
    }

    // Don't run the phishing classifier if the URL came from a private
    // network, since we don't want to ping back in this case.  We also need
    // to check whether the connection was proxied -- if so, we won't have the
    // correct remote IP address, and will skip phishing classification.
    if (params_.was_fetched_via_proxy) {
      VLOG(1) << "Skipping phishing classification for URL: " << params_.url
              << " because it was fetched via a proxy.";
      UMA_HISTOGRAM_ENUMERATION("SBClientPhishing.PreClassificationCheckFail",
                                NO_CLASSIFY_PROXY_FETCH,
                                NO_CLASSIFY_MAX);
      return;
    }
    if (csd_service_->IsPrivateIPAddress(params_.socket_address.host())) {
      VLOG(1) << "Skipping phishing classification for URL: " << params_.url
              << " because of hosting on private IP: "
              << params_.socket_address.host();
      UMA_HISTOGRAM_ENUMERATION("SBClientPhishing.PreClassificationCheckFail",
                                NO_CLASSIFY_PRIVATE_IP,
                                NO_CLASSIFY_MAX);
      return;
    }

    // Don't run the phishing classifier if the tab is incognito.
    if (tab_contents_->profile()->IsOffTheRecord()) {
      VLOG(1) << "Skipping phishing classification for URL: " << params_.url
              << " because we're browsing incognito.";
      UMA_HISTOGRAM_ENUMERATION("SBClientPhishing.PreClassificationCheckFail",
                                NO_CLASSIFY_OFF_THE_RECORD,
                                NO_CLASSIFY_MAX);

      return;
    }

    // We lookup the csd-whitelist before we lookup the cache because
    // a URL may have recently been whitelisted.  If the URL matches
    // the csd-whitelist we won't start classification.  The
    // csd-whitelist check has to be done on the IO thread because it
    // uses the SafeBrowsing service class.
    BrowserThread::PostTask(
        BrowserThread::IO,
        FROM_HERE,
        NewRunnableMethod(this,
                          &ShouldClassifyUrlRequest::CheckCsdWhitelist,
                          params_.url));
  }

  void Cancel() {
    canceled_ = true;
    // Just to make sure we don't do anything stupid we reset all these
    // pointers except for the safebrowsing service class which may be
    // accessed by CheckCsdWhitelist().
    tab_contents_ = NULL;
    csd_service_ = NULL;
    host_ = NULL;
  }

 private:
  friend class base::RefCountedThreadSafe<
      ClientSideDetectionHost::ShouldClassifyUrlRequest>;

  // Enum used to keep stats about why the pre-classification check failed.
  enum PreClassificationCheckFailures {
    NO_CLASSIFY_PROXY_FETCH,
    NO_CLASSIFY_PRIVATE_IP,
    NO_CLASSIFY_OFF_THE_RECORD,
    NO_CLASSIFY_MATCH_CSD_WHITELIST,
    NO_CLASSIFY_TOO_MANY_REPORTS,
    NO_CLASSIFY_UNSUPPORTED_MIME_TYPE,

    NO_CLASSIFY_MAX  // Always add new values before this one.
  };

  // The destructor can be called either from the UI or the IO thread.
  virtual ~ShouldClassifyUrlRequest() { }

  void CheckCsdWhitelist(const GURL& url) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
    if (!sb_service_ || sb_service_->MatchCsdWhitelistUrl(url)) {
      // We're done.  There is no point in going back to the UI thread.
      VLOG(1) << "Skipping phishing classification for URL: " << url
              << " because it matches the csd whitelist";
      UMA_HISTOGRAM_ENUMERATION("SBClientPhishing.PreClassificationCheckFail",
                                NO_CLASSIFY_MATCH_CSD_WHITELIST,
                                NO_CLASSIFY_MAX);
      return;
    }

    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        NewRunnableMethod(this,
                          &ShouldClassifyUrlRequest::CheckCache));
  }

  void CheckCache() {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    if (canceled_) {
      return;
    }

    // If result is cached, we don't want to run classification again
    bool is_phishing;
    if (csd_service_->GetValidCachedResult(params_.url, &is_phishing)) {
      VLOG(1) << "Satisfying request for " << params_.url << " from cache";
      UMA_HISTOGRAM_COUNTS("SBClientPhishing.RequestSatisfiedFromCache", 1);
      // Since we are already on the UI thread, this is safe.
      host_->MaybeShowPhishingWarning(params_.url, is_phishing);
      return;
    }

    // We want to limit the number of requests, though we will ignore the
    // limit for urls in the cache.  We don't want to start classifying
    // too many pages as phishing, but for those that we already think are
    // phishing we want to give ourselves a chance to fix false positives.
    if (csd_service_->IsInCache(params_.url)) {
      VLOG(1) << "Reporting limit skipped for " << params_.url
              << " as it was in the cache.";
      UMA_HISTOGRAM_COUNTS("SBClientPhishing.ReportLimitSkipped", 1);
    } else if (csd_service_->OverReportLimit()) {
      VLOG(1) << "Too many report phishing requests sent recently, "
              << "not running classification for " << params_.url;
      UMA_HISTOGRAM_ENUMERATION("SBClientPhishing.PreClassificationCheckFail",
                                NO_CLASSIFY_TOO_MANY_REPORTS,
                                NO_CLASSIFY_MAX);
      return;
    }

    // Everything checks out, so start classification.
    // |tab_contents_| is safe to call as we will be destructed
    // before it is.
    VLOG(1) << "Instruct renderer to start phishing detection for URL: "
            << params_.url;
    RenderViewHost* rvh = tab_contents_->render_view_host();
    rvh->Send(new SafeBrowsingMsg_StartPhishingDetection(
        rvh->routing_id(), params_.url));
  }

  // No need to protect |canceled_| with a lock because it is only read and
  // written by the UI thread.
  bool canceled_;
  ViewHostMsg_FrameNavigate_Params params_;
  TabContents* tab_contents_;
  ClientSideDetectionService* csd_service_;
  // We keep a ref pointer here just to make sure the service class stays alive
  // long enough.
  scoped_refptr<SafeBrowsingService> sb_service_;
  ClientSideDetectionHost* host_;

  DISALLOW_COPY_AND_ASSIGN(ShouldClassifyUrlRequest);
};

// This class is used to display the phishing interstitial.
class CsdClient : public SafeBrowsingService::Client {
 public:
  CsdClient() {}

  // Method from SafeBrowsingService::Client.  This method is called on the
  // IO thread once the interstitial is going away.  This method simply deletes
  // the CsdClient object.
  virtual void OnBlockingPageComplete(bool proceed) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
    // Delete this on the UI thread since it was created there.
    BrowserThread::PostTask(BrowserThread::UI,
                            FROM_HERE,
                            new DeleteTask<CsdClient>(this));
  }

 private:
  friend class DeleteTask<CsdClient>;  // Calls the private destructor.

  // We're taking care of deleting this object.  No-one else should delete
  // this object.
  virtual ~CsdClient() {}

  DISALLOW_COPY_AND_ASSIGN(CsdClient);
};

// static
ClientSideDetectionHost* ClientSideDetectionHost::Create(
    TabContents* tab) {
  return new ClientSideDetectionHost(tab);
}

ClientSideDetectionHost::ClientSideDetectionHost(TabContents* tab)
    : TabContentsObserver(tab),
      csd_service_(g_browser_process->safe_browsing_detection_service()),
      feature_extractor_(
          new BrowserFeatureExtractor(
              tab,
              g_browser_process->safe_browsing_detection_service())),
      cb_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)) {
  DCHECK(tab);
  // Note: csd_service_ and sb_service_ might be NULL.
  sb_service_ = g_browser_process->safe_browsing_service();
  registrar_.Add(this, content::NOTIFICATION_RESOURCE_RESPONSE_STARTED,
                 Source<RenderViewHostDelegate>(tab));
}

ClientSideDetectionHost::~ClientSideDetectionHost() {}

bool ClientSideDetectionHost::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ClientSideDetectionHost, message)
    IPC_MESSAGE_HANDLER(SafeBrowsingHostMsg_DetectedPhishingSite,
                        OnDetectedPhishingSite)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void ClientSideDetectionHost::DidNavigateMainFramePostCommit(
    const content::LoadCommittedDetails& details,
    const ViewHostMsg_FrameNavigate_Params& params) {
  // TODO(noelutz): move this DCHECK to TabContents and fix all the unit tests
  // that don't call this method on the UI thread.
  // DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (details.is_in_page) {
    // If the navigation is within the same page, the user isn't really
    // navigating away.  We don't need to cancel a pending callback or
    // begin a new classification.
    return;
  }
  // If we navigate away and there currently is a pending phishing
  // report request we have to cancel it to make sure we don't display
  // an interstitial for the wrong page.  Note that this won't cancel
  // the server ping back but only cancel the showing of the
  // interstial.
  cb_factory_.RevokeAll();

  if (!csd_service_) {
    return;
  }

  // Cancel any pending classification request.
  if (classification_request_.get()) {
    classification_request_->Cancel();
  }
  browse_info_.reset(new BrowseInfo);
  browse_info_->url = params.url;
  browse_info_->referrer = params.referrer;
  browse_info_->transition = params.transition;

  // Notify the renderer if it should classify this URL.
  classification_request_ = new ShouldClassifyUrlRequest(params,
                                                         tab_contents(),
                                                         csd_service_,
                                                         sb_service_,
                                                         this);
  classification_request_->Start();
}

void ClientSideDetectionHost::TabContentsDestroyed(TabContents* tab) {
  DCHECK(tab);
  // Tell any pending classification request that it is being canceled.
  if (classification_request_.get()) {
    classification_request_->Cancel();
  }
  // Cancel all pending feature extractions.
  feature_extractor_.reset();
}

void ClientSideDetectionHost::OnDetectedPhishingSite(
    const std::string& verdict_str) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // There is something seriously wrong if there is no service class but
  // this method is called.  The renderer should not start phishing detection
  // if there isn't any service class in the browser.
  DCHECK(csd_service_);
  // There shouldn't be any pending requests because we revoke them everytime
  // we navigate away.
  DCHECK(!cb_factory_.HasPendingCallbacks());
  DCHECK(browse_info_.get());

  // We parse the protocol buffer here.  If we're unable to parse it we won't
  // send the verdict further.
  scoped_ptr<ClientPhishingRequest> verdict(new ClientPhishingRequest);
  if (csd_service_ &&
      !cb_factory_.HasPendingCallbacks() &&
      browse_info_.get() &&
      verdict->ParseFromString(verdict_str) &&
      verdict->IsInitialized()) {
    if (browse_info_->url.spec() != verdict->url()) {
      // I'm not sure we can DCHECK on this one so we keep stats around to see
      // whether this actually happens in practice.
      UMA_HISTOGRAM_COUNTS("SBClientPhishing.BrowserRendererUrlMismatch", 1);
      VLOG(2) << "Browser and renderer URL do not match: "
              << browse_info_->url.spec() << " vs. " << verdict->url();
    }
    // Start browser-side feature extraction.  Once we're done it will send
    // the client verdict request.
    feature_extractor_->ExtractFeatures(
        browse_info_.get(),
        verdict.release(),
        NewCallback(this, &ClientSideDetectionHost::FeatureExtractionDone));
  }
  browse_info_.reset();
}

void ClientSideDetectionHost::MaybeShowPhishingWarning(GURL phishing_url,
                                                       bool is_phishing) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  VLOG(2) << "Received server phishing verdict for URL:" << phishing_url
          << " is_phishing:" << is_phishing;
  if (is_phishing) {
    DCHECK(tab_contents());
    if (sb_service_) {
      SafeBrowsingService::UnsafeResource resource;
      resource.url = phishing_url;
      resource.original_url = phishing_url;
      resource.is_subresource = false;
      resource.threat_type = SafeBrowsingService::CLIENT_SIDE_PHISHING_URL;
      resource.render_process_host_id =
          tab_contents()->GetRenderProcessHost()->id();
      resource.render_view_id =
          tab_contents()->render_view_host()->routing_id();
      if (!sb_service_->IsWhitelisted(resource)) {
        // We need to stop any pending navigations, otherwise the interstital
        // might not get created properly.
        tab_contents()->controller().DiscardNonCommittedEntries();
        resource.client = new CsdClient();  // Will delete itself
        sb_service_->DoDisplayBlockingPage(resource);
      }
    }
  }
}

void ClientSideDetectionHost::FeatureExtractionDone(
    bool success,
    ClientPhishingRequest* request) {
  if (!request) {
    DLOG(FATAL) << "Invalid request object in FeatureExtractionDone";
    return;
  }
  VLOG(2) << "Feature extraction done (success:" << success << ") for URL: "
          << request->url() << ". Start sending client phishing request.";
  // Send ping no matter what - even if the browser feature extraction failed.
  csd_service_->SendClientReportPhishingRequest(
      request,  // The service takes ownership of the request object.
      cb_factory_.NewCallback(
          &ClientSideDetectionHost::MaybeShowPhishingWarning));
}

void ClientSideDetectionHost::Observe(int type,
                                      const NotificationSource& source,
                                      const NotificationDetails& details) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK_EQ(type, content::NOTIFICATION_RESOURCE_RESPONSE_STARTED);
  const ResourceRequestDetails* req = Details<ResourceRequestDetails>(
      details).ptr();
  if (req && browse_info_.get()) {
    browse_info_->ips.insert(req->socket_address().host());
  }
}

void ClientSideDetectionHost::set_client_side_detection_service(
    ClientSideDetectionService* service) {
  csd_service_ = service;
}

void ClientSideDetectionHost::set_safe_browsing_service(
    SafeBrowsingService* service) {
  sb_service_ = service;
}

}  // namespace safe_browsing
