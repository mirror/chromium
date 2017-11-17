// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <atomic>

#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "content/browser/frame_host/navigation_controller_impl.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/ui_test_utils.h"
#include "extensions/browser/api/web_request/web_request_api.h"
#include "net/dns/mock_host_resolver.h"
#include "base/threading/platform_thread.h"

namespace extensions {
namespace {

// Top 500 sites as per https://moz.com/top500.
const char* kTop500Domains[] = {
    "https://google.com/"
    , "https://youtube.com/", "https://linkedin.com/",
    "https://wordpress.org/",
    "https://instagram.com/",
    "https://pinterest.com/",
    "https://wikipedia.org/",
    "https://wordpress.com/",
    "https://blogspot.com/",
    "https://apple.com/",
    "https://adobe.com/",
    "https://tumblr.com/",
    "https://youtu.be/",
    "https://amazon.com/",
    "https://vimeo.com/",
    "https://goo.gl/",
    "https://microsoft.com/",
    "https://yahoo.com/",
    "https://flickr.com/",
    "https://qq.com/",
    "https://bit.ly/",
    "https://godaddy.com/",
    "https://buydomains.com/",
    "https://vk.com/",
    "https://baidu.com/",
    "https://reddit.com/",
    "https://w3.org/",
    "https://nytimes.com/",
    "https://t.co/",
    "https://europa.eu/",
    "https://weebly.com/",
    "https://statcounter.com/",
    "https://gov.uk/",
    "https://bbc.co.uk/",
    "https://mozilla.org/",
    "https://myspace.com/",
    "https://yandex.ru/",
    "https://google.de/",
    "https://blogger.com/",
    "https://soundcloud.com/",
    "https://123-reg.co.uk/",
    "https://github.com/",
    "https://miitbeian.gov.cn/",
    "https://wp.com/",
    "https://jimdo.com/",
    "https://theguardian.com/",
    "https://google.co.jp/",
    "https://addthis.com/",
    "https://wix.com/",
    "https://cnn.com/",
    "https://paypal.com/",
    "https://nih.gov/",
    "https://bluehost.com/",
    "https://creativecommons.org/",
    "https://wixsite.com/",
    "https://whoisprivacyprotect.com/",
    "https://digg.com/",
    "https://stumbleupon.com/",
    "https://huffingtonpost.com/",
    "https://issuu.com/",
    "https://gravatar.com/",
    "https://imdb.com/",
    "https://ascii.co.uk/",
    "https://feedburner.com/",
    "https://yelp.com/",
    "https://parallels.com/",
    "https://google.co.uk/",
    "https://dropbox.com/",
    "https://amazonaws.com/",
    "https://forbes.com/",
    "https://tinyurl.com/",
    "https://miibeian.gov.cn/",
    "https://msn.com/",
    "https://go.com/",
    "https://wsj.com/",
    "https://e-recht24.de/",
    "https://addtoany.com/",
    "https://slideshare.net/",
    "https://washingtonpost.com/",
    "https://etsy.com/",
    "https://weibo.com/",
    "https://archive.org/",
    "https://ameblo.jp/",
    "https://bing.com/",
    "https://eventbrite.com/",
    "https://fc2.com/",
    "https://ebay.com/",
    "https://free.fr/",
    "https://telegraph.co.uk/",
    "https://sourceforge.net/",
    "https://51.la/",
    "https://taobao.com/",
    "https://amazon.co.uk/",
    "https://livejournal.com/",
    "https://mail.ru/",
    "https://about.com/",
    "https://reuters.com/",
    "https://yahoo.co.jp/",
    "https://typepad.com/",
    "https://dailymail.co.uk/",
    "https://cpanel.net/",
    "https://bloomberg.com/",
    "https://aol.com/",
    "https://macromedia.com/",
    "https://amazon.de/",
    "https://usatoday.com/",
    "https://wikimedia.org/",
    "https://cpanel.com/",
    "https://sina.com.cn/",
    "https://time.com/",
    "https://eepurl.com/",
    "https://cdc.gov/",
    "https://amzn.to/",
    "https://webs.com/",
    "https://constantcontact.com/",
    "https://xing.com/",
    "https://opera.com/",
    "https://live.com/",
    "https://harvard.edu/",
    "https://google.it/",
    "https://npr.org/",
    "https://dailymotion.com/",
    "https://bbb.org/",
    "https://latimes.com/",
    "https://tripadvisor.com/",
    "https://hatena.ne.jp/",
    "https://blogspot.co.uk/",
    "https://icio.us/",
    "https://doubleclick.net/",
    "https://elegantthemes.com/",
    "https://apache.org/",
    "https://domainactive.co/",
    "https://bandcamp.com/",
    "https://amazon.co.jp/",
    "https://list-manage.com/",
    "https://mit.edu/",
    "https://google.es/",
    "https://gnu.org/",
    "https://google.fr/",
    "https://guardian.co.uk/",
    "https://surveymonkey.com/",
    "https://beian.gov.cn/",
    "https://behance.net/",
    "https://joomla.org/",
    "https://businessinsider.com/",
    "https://medium.com/",
    "https://stanford.edu/",
    "https://bbc.com/",
    "https://rambler.ru/",
    "https://nasa.gov/",
    "https://kickstarter.com/",
    "https://geocities.com/",
    "https://wired.com/",
    "https://delicious.com/",
    "https://ted.com/",
    "https://googleusercontent.com/",
    "https://google.ca/",
    "https://tripod.com/",
    "https://independent.co.uk/",
    "https://cnet.com/",
    "https://disqus.com/",
    "https://imgur.com/",
    "https://github.io/",
    "https://photobucket.com/",
    "https://scribd.com/",
    "https://namejet.com/",
    "https://goodreads.com/",
    "https://homestead.com/",
    "https://deviantart.com/",
    "https://163.com/",
    "https://vkontakte.ru/",
    "https://rakuten.co.jp/",
    "https://spotify.com/",
    "https://ca.gov/",
    "https://1und1.de/",
    "https://secureserver.net/",
    "https://themeforest.net/",
    "https://barnesandnoble.com/",
    "https://nationalgeographic.com/",
    "https://who.int/",
    "https://wiley.com/",
    "https://pbs.org/",
    "https://un.org/",
    "https://plesk.com/",
    "https://buzzfeed.com/",
    "https://jiathis.com/",
    "https://cbsnews.com/",
    "https://4.cn/",
    "https://webmd.com/",
    "https://one.com/",
    "https://nbcnews.com/",
    "https://foxnews.com/",
    "https://sakura.ne.jp/",
    "https://berkeley.edu/",
    "https://ibm.com/",
    "https://blogspot.com.es/",
    "https://theatlantic.com/",
    "https://hostnet.nl/",
    "https://mashable.com/",
    "https://trustpilot.com/",
    "https://sohu.com/",
    "https://nature.com/",
    "https://networksolutions.com/",
    "https://php.net/",
    "https://line.me/",
    "https://whitehouse.gov/",
    "https://techcrunch.com/",
    "https://usda.gov/",
    "https://cornell.edu/",
    "https://sciencedirect.com/",
    "https://mapquest.com/",
    "https://squarespace.com/",
    "https://domainname.de/",
    "https://epa.gov/",
    "https://noaa.gov/",
    "https://blogspot.de/",
    "https://change.org/",
    "https://blogspot.ca/",
    "https://skype.com/",
    "https://ow.ly/",
    "https://cbc.ca/",
    "https://directdomains.com/",
    "https://wp.me/",
    "https://nps.gov/",
    "https://a8.net/",
    "https://meetup.com/",
    "https://springer.com/",
    "https://ft.com/",
    "https://booking.com/",
    "https://phoca.cz/",
    "https://cloudfront.net/",
    "https://wikia.com/",
    "https://usnews.com/",
    "https://google.nl/",
    "https://loc.gov/",
    "https://foursquare.com/",
    "https://wufoo.com/",
    "https://fda.gov/",
    "https://economist.com/",
    "https://sfgate.com/",
    "https://cbslocal.com/",
    "https://myshopify.com/",
    "https://blogspot.fr/",
    "https://bizjournals.com/",
    "https://google.com.br/",
    "https://uol.com.br/",
    "https://nhs.uk/",
    "https://spiegel.de/",
    "https://irs.gov/",
    "https://cnbc.com/",
    "https://xinhuanet.com/",
    "https://list-manage1.com/",
    "https://phpbb.com/",
    "https://naver.com/",
    "https://abc.net.au/",
    "https://chicagotribune.com/",
    "https://gizmodo.com/",
    "https://mijndomein.nl/",
    "https://hp.com/",
    "https://enable-javascript.com/",
    "https://google.com.au/",
    "https://geocities.jp/",
    "https://prnewswire.com/",
    "https://bestfwdservice.com/",
    "https://clickbank.net/",
    "https://bigcartel.com/",
    "https://engadget.com/",
    "https://marriott.com/",
    "https://slate.com/",
    "https://loopia.se/",
    "https://about.me/",
    "https://newyorker.com/",
    "https://loopia.com/",
    "https://visma.com/",
    "https://nifty.com/",
    "https://umblr.com/",
    "https://nydailynews.com/",
    "https://ed.gov/",
    "https://histats.com/",
    "https://weather.com/",
    "https://state.gov/",
    "https://livedoor.jp/",
    "https://umich.edu/",
    "https://storify.com/",
    "https://acquirethisname.com/",
    "https://sogou.com/",
    "https://fortune.com/",
    "https://dribbble.com/",
    "https://shopify.com/",
    "https://youku.com/",
    "https://house.gov/",
    "https://vice.com/",
    "https://ocn.ne.jp/",
    "https://unesco.org/",
    "https://indiegogo.com/",
    "https://1and1.fr/",
    "https://hilton.com/",
    "https://yale.edu/",
    "https://fb.com/",
    "https://wunderground.com/",
    "https://1and1.com/",
    "https://aboutcookies.org/",
    "https://houzz.com/",
    "https://google.pl/",
    "https://sedo.com/",
    "https://doi.org/",
    "https://columbia.edu/",
    "https://oup.com/",
    "https://linksynergy.com/",
    "https://indiatimes.com/",
    "https://mayoclinic.org/",
    "https://businessweek.com/",
    "https://biglobe.ne.jp/",
    "https://marketwatch.com/",
    "https://rs6.net/",
    "https://home.pl/",
    "https://ustream.tv/",
    "https://globo.com/",
    "https://fb.me/",
    "https://sciencedaily.com/",
    "https://washington.edu/",
    "https://samsung.com/",
    "https://upenn.edu/",
    "https://academia.edu/",
    "https://examiner.com/",
    "https://1688.com/",
    "https://debian.org/",
    "https://espn.com/",
    "https://android.com/",
    "https://smh.com.au/",
    "https://goo.ne.jp/",
    "https://wikihow.com/",
    "https://blogspot.in/",
    "https://oracle.com/",
    "https://networkadvertising.org/",
    "https://allaboutcookies.org/",
    "https://gofundme.com/",
    "https://senate.gov/",
    "https://alibaba.com/",
    "https://theverge.com/",
    "https://boston.com/",
    "https://telegram.me/",
    "https://youronlinechoices.com/",
    "https://psychologytoday.com/",
    "https://ftc.gov/",
    "https://archives.gov/",
    "https://zdnet.com/",
    "https://fbcdn.net/",
    "https://scientificamerican.com/",
    "https://mailchimp.com/",
    "https://theglobeandmail.com/",
    "https://nypost.com/",
    "https://ucla.edu/",
    "https://elpais.com/",
    "https://ok.ru/",
    "https://thetimes.co.uk/",
    "https://feedly.com/",
    "https://cdbaby.com/",
    "https://wisc.edu/",
    "https://themegrill.com/",
    "https://umn.edu/",
    "https://fastcompany.com/",
    "https://sagepub.com/",
    "https://utexas.edu/",
    "https://psu.edu/",
    "https://mozilla.com/",
    "https://businesswire.com/",
    "https://studiopress.com/",
    "https://walmart.com/",
    "https://hibu.com/",
    "https://google.co.in/",
    "https://ox.ac.uk/",
    "https://drupal.org/",
    "https://dreamhost.com/",
    "https://prweb.com/",
    "https://entrepreneur.com/",
    "https://exblog.jp/",
    "https://alexa.com/",
    "https://icann.org/",
    "https://domraider.io/",
    "https://campaign-archive1.com/",
    "https://census.gov/",
    "https://tripadvisor.co.uk/",
    "https://hbr.org/",
    "https://si.edu/",
    "https://inc.com/",
    "https://campaign-archive2.com/",
    "https://sciencemag.org/",
    "https://mirror.co.uk/",
    "https://dropcatch.com/",
    "https://worldbank.org/",
    "https://shareasale.com/",
    "https://newsweek.com/",
    "https://nazwa.pl/",
    "https://usgs.gov/",
    "https://intel.com/",
    "https://shinystat.com/",
    "https://mysql.com/",
    "https://target.com/",
    "https://hostgator.com/",
    "https://seesaa.net/",
    "https://axs.com/",
    "https://nymag.com/",
    "https://researchgate.net/",
    "https://rollingstone.com/",
    "https://jugem.jp/",
    "https://office.com/",
    "https://princeton.edu/",
    "https://example.com/",
    "https://ebay.co.uk/",
    "https://tandfonline.com/",
    "https://stackoverflow.com/",
    "https://cisco.com/",
    "https://smugmug.com/",
    "https://360.cn/",
    "https://hhs.gov/",
    "https://lemonde.fr/",
    "https://nyu.edu/",
    "https://ap.org/",
    "https://amazon.fr/",
    "https://thedailybeast.com/",
    "https://cam.ac.uk/",
    "https://detik.com/",
    "https://arstechnica.com/",
    "https://dell.com/",
    "https://box.com/",
    "https://namebright.com/",
    "https://dropboxusercontent.com/",
    "https://hubspot.com/",
    "https://politico.com/",
    "https://accuweather.com/",
    "https://t-online.de/",
    "https://mlb.com/",
    "https://lifehacker.com/",
    "https://oxfordjournals.org/",
    "https://list-manage2.com/",
    "https://zendesk.com/",
    "https://hollywoodreporter.com/",
    "https://cryoutcreations.eu/",
    "https://istockphoto.com/",
    "https://fotolia.com/",
    "https://teamviewer.com/",
    "https://va.gov/",
    "https://cmu.edu/",
    "https://bls.gov/",
    "https://uchicago.edu/",
    "https://admin.ch/",
    "https://xiti.com/",
    "https://google.ru/",
    "https://athemes.com/",
    "https://domainnameshop.com/",
    "https://nhk.or.jp/",
    "https://aliyun.com/",
    "https://shop-pro.jp/",
    "https://openstreetmap.org/",
    "https://bmj.com/",
    "https://tucowsdomains.com/",
    "https://oecd.org/",
    "https://apa.org/",
    "https://redcross.org/",
    "https://news.com.au/",
    "https://domeneshop.no/",
    "https://livestream.com/",
    "https://umd.edu/",
    "https://netflix.com/",
    "https://variety.com/",
    "https://presscustomizr.com/",
    "https://prestashop.com/",
    "https://chron.com/",
    "https://usc.edu/",
    "https://wsimg.com/",
    "https://att.com/",
    "https://nsw.gov.au/",
    "https://adweek.com/",
    "https://ewebdevelopment.com/",
    "https://cafepress.com/",
    "https://hud.gov/",
    "https://dot.gov/",
    "https://blogspot.jp/",
    "https://unc.edu/",
    "https://usa.gov/",
    "https://eventbrite.co.uk/",
    "https://state.tx.us/",
    "https://twitch.tv/",
    "https://opensource.org/",
    "https://symantec.com/",
    "https://windowsphone.com/",
    "https://ieee.org/",
    "https://nginx.org/",
    "https://com.com/",
    "https://aljazeera.com/",
    "https://today.com/",
    "https://washingtontimes.com/",
    "https://aboutads.info/",
    "https://500px.com/",
    "https://mtv.com/",
    "https://staticflickr.com/",
    "https://army.mil/",
};

class TabCreationObserver : public TabStripModelObserver {
 public:
  TabCreationObserver() {}

  void Wait() {
    if (!seen_)
      run_loop_.Run();
  }

  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override {
    run_loop_.Quit();
    seen_ = true;
  }

 private:
  bool seen_ = false;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(TabCreationObserver);
};

class WebRequestPerformanceTest
    : public ExtensionBrowserTest,
      public ExtensionWebRequestEventRouter::TestObserver {
 public:
  WebRequestPerformanceTest() {}

 protected:
  void StartTests(const std::string& extension_name) {
    content::NavigationControllerImpl::set_max_entry_count_for_testing(1024);
    content::RunAllTasksUntilIdle();
    ExtensionWebRequestEventRouter::GetInstance()->SetObserverForTest(this);

    base::ElapsedTimer timer;
    for (const auto* url : kTop500Domains)
      ui_test_utils::NavigateToURL(browser(), GURL(url));

    // Wait for some time hoping any pending network requests get completed.
    base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(5));

    total_test_duration_ = timer.Elapsed();

    ExtensionWebRequestEventRouter::GetInstance()->SetObserverForTest(nullptr);

    LogResults(extension_name);
  }

  void set_allow_request_end_with_no_start() {
    allow_request_end_with_no_start_ = true;
  }
 private:
  struct URLRequestData {
    base::TimeTicks start_time_cpu_;
    base::TimeTicks end_time_cpu_;
    bool active;
    base::ThreadTicks start_time_thread_;
    base::ThreadTicks end_time_thread_;
    bool cancelled;
    bool redirected;
  };

  // BrowserTestBase override
  void SetUpInProcessBrowserTestFixture() override {
    // To avoid depending on external resources, browser tests don't allow
    // non-local DNS queries by default. Override this and allow connection to
    // all external hosts.
    auto resolver =
        base::MakeRefCounted<net::RuleBasedHostResolverProc>(host_resolver());
    resolver->AllowDirectLookup("*");
    mock_host_resolver_override_ =
        std::make_unique<net::ScopedDefaultHostResolverProc>(resolver.get());
  }

  // ExtensionWebRequestEventRouter::TestObserver overrides:
  void OnRequestWasBlocked(const net::URLRequest& request) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    CHECK(base::ContainsKey(request_data_map_, request.identifier()));
    request_data_map_[request.identifier()].cancelled = true;
  }

  void OnRequestWasRedirected(const net::URLRequest& request) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    CHECK(base::ContainsKey(request_data_map_, request.identifier()));
    request_data_map_[request.identifier()].redirected = true;
  }

  void OnBeforeRequest(const net::URLRequest& request) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

    // Can happen for redirects etc.
    if (base::ContainsKey(request_data_map_, request.identifier()))
        return;

    active_request_count_++;
    request_data_map_[request.identifier()] = {
        base::TimeTicks::Now(),
        base::TimeTicks(),
        true,
        base::ThreadTicks::Now(),
        base::ThreadTicks(),
        false,
        false,
    };
  }

  void OnRequestEnded(const net::URLRequest& request) override {
    if (!base::ContainsKey(request_data_map_, request.identifier()))
        return;

    DCHECK(request_data_map_[request.identifier()].active);

    active_request_count_--;

    request_data_map_[request.identifier()].active = false;
    request_data_map_[request.identifier()].end_time_cpu_ = base::TimeTicks::Now();
    request_data_map_[request.identifier()].end_time_thread_ = base::ThreadTicks::Now();
  }

  void LogResults(const std::string& extension_name) {
    base::TimeDelta total_cpu_duration;
    base::TimeDelta total_thread_duration;
    size_t count_requests_blocked = 0;
    size_t count_requests_redirected = 0;
    size_t request_count = request_data_map_.size();

    for (const auto& val : request_data_map_) {
      const URLRequestData& data = val.second;
      if (data.active) {
        request_count--;
        continue;
      }

      if (data.cancelled)
        count_requests_blocked++;
      if (data.redirected)
        count_requests_redirected++;
      total_cpu_duration += (data.end_time_cpu_ - data.start_time_cpu_);
      total_thread_duration +=
          (data.end_time_thread_ - data.start_time_thread_);
    }



    LOG(ERROR) << "--------Results for " << extension_name;
    LOG(ERROR) << "--------total_test_duration_ " << total_test_duration_;
    LOG(ERROR) << base::StringPrintf("%zu/%zu requests were blocked.",
                                     count_requests_blocked, request_count);
    LOG(ERROR) << base::StringPrintf("%zu/%zu requests were redirected.",
                                     count_requests_redirected, request_count);
    LOG(ERROR) << "--------Average CPU time per request "
               << (total_cpu_duration * 1.0f) / request_count;
    LOG(ERROR) << "--------Average thread time per request "
               << (total_thread_duration * 1.0f) / request_count;
  }

  std::unique_ptr<net::ScopedDefaultHostResolverProc>
      mock_host_resolver_override_;
  base::TimeDelta total_test_duration_;

  // Map from request identifiers to URLRequestData.
  std::map<uint64_t, URLRequestData> request_data_map_;

  // Modified on the IO thread, also read on UI.
  std::atomic<size_t> active_request_count_;

  bool allow_request_end_with_no_start_ = false;

  DISALLOW_COPY_AND_ASSIGN(WebRequestPerformanceTest);
};

// Tests performance of a DNR extension containing easylist blocking rules. No
// redirect rules, no $document rules, no regex rules.
IN_PROC_BROWSER_TEST_F(WebRequestPerformanceTest, DNREasylistExtension) {
  base::FilePath extension_path;
  PathService::Get(chrome::DIR_TEST_DATA, &extension_path);
  extension_path = extension_path.AppendASCII("extensions")
                       .AppendASCII("declarative_net_request")
                       .AppendASCII("dwr_unpacked");
  const Extension* extension = LoadExtension(extension_path);
  ASSERT_TRUE(extension);

  StartTests(extension->name());
}

IN_PROC_BROWSER_TEST_F(WebRequestPerformanceTest, AdblockPlus) {
  base::FilePath extension_path;
  PathService::Get(chrome::DIR_TEST_DATA, &extension_path);

  // This is a Dev build obtained from
  // https://downloads.adblockplus.org/devbuilds/adblockpluschrome/.
  extension_path = extension_path.AppendASCII("extensions")
                       .AppendASCII("declarative_net_request")
                       .AppendASCII("adblockpluschrome-3.0.1.1926.crx");

  TabCreationObserver observer;
  browser()->tab_strip_model()->AddObserver(&observer);

  // Ignore manifest warnings due to unrecognized keys.
  const Extension* extension =
      LoadExtensionWithFlags(extension_path, kFlagIgnoreManifestWarnings);
  ASSERT_TRUE(extension);

  // Adblock will launch a tab on installation. Wait for it.
  // content::BlockAllTasksUntilIdle doesn't work for this, probably since the
  // browser is idle, but the JS in the extension renderer causes a new task to
  // be posted to the browser after some time.
  observer.Wait();
  browser()->tab_strip_model()->RemoveObserver(&observer);

  EXPECT_EQ(2, browser()->tab_strip_model()->count());

  // Close the active tab.
  browser()->tab_strip_model()->CloseWebContentsAt(
      browser()->tab_strip_model()->active_index(), TabStripModel::CLOSE_NONE);

  EXPECT_EQ(1, browser()->tab_strip_model()->count());

  set_allow_request_end_with_no_start();

  StartTests(extension->name());
}

// Tests performance of a DNR extension containing easylist blocking rules. No
// redirect rules, no $document rules, no regex rules.
IN_PROC_BROWSER_TEST_F(WebRequestPerformanceTest, Ublock) {
  base::FilePath extension_path;
  PathService::Get(chrome::DIR_TEST_DATA, &extension_path);
  // Unpacked ublock dev version obtained from
  // https://github.com/gorhill/uBlock/releases/tag/1.14.19b7.
  extension_path = extension_path.AppendASCII("extensions")
                       .AppendASCII("declarative_net_request")
                       .AppendASCII("ublock_dev");
  const Extension* extension = LoadExtension(extension_path);
  ASSERT_TRUE(extension);

  StartTests(extension->name());
}

}  // namespace
}  // namespace extensions
