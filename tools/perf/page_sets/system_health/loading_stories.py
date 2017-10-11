# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from page_sets.system_health import platforms
from page_sets.system_health import story_tags
from page_sets.system_health import system_health_story

from page_sets.login_helpers import dropbox_login
from page_sets.login_helpers import google_login


class _LoadingStory(system_health_story.SystemHealthStory):
  """Abstract base class for single-page System Health user stories."""
  ABSTRACT_STORY = True

  @classmethod
  def GenerateStoryDescription(cls):
    return 'Load %s' % cls.URL


################################################################################
# Search and e-commerce.
################################################################################
# TODO(petrcermak): Split these into 'portal' and 'shopping' stories.


class LoadGoogleStory(_LoadingStory):
  NAME = 'load:search:google'
  URL = 'https://www.google.co.uk/'


class LoadBaiduStory(_LoadingStory):
  NAME = 'load:search:baidu'
  URL = 'https://www.baidu.com/s?word=google'
  TAGS = [story_tags.INTERNATIONAL]


class LoadYahooStory(_LoadingStory):
  NAME = 'load:search:yahoo'
  URL = 'https://search.yahoo.com/search;_ylt=?p=google'


class LoadAmazonDesktopStory(_LoadingStory):
  NAME = 'load:search:amazon'
  URL = 'https://www.amazon.com/s/?field-keywords=nexus'
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY


class LoadTaobaoDesktopStory(_LoadingStory):
  NAME = 'load:search:taobao'
  URL = 'https://world.taobao.com/'
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY
  TAGS = [story_tags.INTERNATIONAL]


class LoadTaobaoMobileStory(_LoadingStory):
  NAME = 'load:search:taobao'
  # "ali_trackid" in the URL suppresses "Download app" interstitial.
  URL = 'http://m.intl.taobao.com/?ali_trackid'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY
  TAGS = [story_tags.INTERNATIONAL]


class LoadYandexStory(_LoadingStory):
  NAME = 'load:search:yandex'
  URL = 'https://yandex.ru/touchsearch?text=science'
  TAGS = [story_tags.INTERNATIONAL]


class LoadEbayStory(_LoadingStory):
  NAME = 'load:search:ebay'
  # Redirects to the "http://" version.
  URL = 'https://www.ebay.com/sch/i.html?_nkw=headphones'


################################################################################
# Social networks.
################################################################################


class LoadTwitterStory(_LoadingStory):
  NAME = 'load:social:twitter'
  URL = 'https://www.twitter.com/nasa'

  # Desktop version is already covered by
  # 'browse:social:twitter_infinite_scroll'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY


class LoadVkStory(_LoadingStory):
  NAME = 'load:social:vk'
  URL = 'https://vk.com/sbeatles'
  # Due to the deterministic date injected by WPR (February 2008), the cookie
  # set by https://vk.com immediately expires, so the page keeps refreshing
  # indefinitely on mobile
  # (see https://github.com/chromium/web-page-replay/issues/71).
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY
  TAGS = [story_tags.INTERNATIONAL]


class LoadInstagramDesktopStory(_LoadingStory):
  NAME = 'load:social:instagram'
  URL = 'https://www.instagram.com/selenagomez/'
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY


class LoadPinterestStory(_LoadingStory):
  NAME = 'load:social:pinterest'
  URL = 'https://uk.pinterest.com/categories/popular/'
  TAGS = [story_tags.JAVASCRIPT_HEAVY]
  # Mobile story is already covered by
  # 'browse:social:pinterest_infinite_scroll'.
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY


################################################################################
# News, discussion and knowledge portals and blogs.
################################################################################


class LoadBbcDesktopStory(_LoadingStory):
  NAME = 'load:news:bbc'
  # Redirects to the "http://" version.
  URL = 'https://www.bbc.co.uk/news/world-asia-china-36189636'
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY


class LoadCnnStory(_LoadingStory):
  NAME = 'load:news:cnn'
  # Using "https://" shows "Your connection is not private".
  URL = 'http://edition.cnn.com'
  TAGS = [story_tags.JAVASCRIPT_HEAVY]


class LoadFlipboardDesktopStory(_LoadingStory):
  NAME = 'load:news:flipboard'
  URL = 'https://flipboard.com/explore'
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY


class LoadHackerNewsDesktopStory(_LoadingStory):
  NAME = 'load:news:hackernews'
  URL = 'https://news.ycombinator.com'
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY


class LoadNytimesDesktopStory(_LoadingStory):
  NAME = 'load:news:nytimes'
  URL = 'http://www.nytimes.com'
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY


class LoadNytimesMobileStory(_LoadingStory):
  NAME = 'load:news:nytimes'
  URL = 'http://mobile.nytimes.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY


class LoadQqMobileStory(_LoadingStory):
  NAME = 'load:news:qq'
  # Using "https://" hangs and shows "This site can't be reached".
  URL = 'http://news.qq.com'
  TAGS = [story_tags.INTERNATIONAL]


class LoadRedditDesktopStory(_LoadingStory):
  NAME = 'load:news:reddit'
  URL = 'https://www.reddit.com/r/news/top/?sort=top&t=week'
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY


class LoadRedditMobileStory(_LoadingStory):
  NAME = 'load:news:reddit'
  URL = 'https://www.reddit.com/r/news/top/?sort=top&t=week'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY


class LoadWashingtonPostMobileStory(_LoadingStory):
  NAME = 'load:news:washingtonpost'
  URL = 'https://www.washingtonpost.com/pwa'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY
  _CLOSE_BUTTON_SELECTOR = '.close'

  def _DidLoadDocument(self, action_runner):
    # Close the popup window. On Nexus 9 (and probably other tables) the popup
    # window does not have a "Close" button, instead it has only a "Send link
    # to phone" button. So on tablets we run with the popup window open. The
    # popup is transparent, so this is mostly an aesthetical issue.
    has_button = action_runner.EvaluateJavaScript(
        '!!document.querySelector({{ selector }})',
        selector=self._CLOSE_BUTTON_SELECTOR)
    if has_button:
      action_runner.ClickElement(selector=self._CLOSE_BUTTON_SELECTOR)


class LoadWikipediaStory(_LoadingStory):
  NAME = 'load:news:wikipedia'
  URL = 'https://en.wikipedia.org/wiki/Science'
  TAGS = [story_tags.EMERGING_MARKET]


class LoadIrctcStory(_LoadingStory):
  NAME = 'load:news:irctc'
  URL = 'https://www.irctc.co.in'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY
  TAGS = [story_tags.EMERGING_MARKET]


################################################################################
# Audio, images, and video.
################################################################################


class LoadYouTubeStory(_LoadingStory):
  # No way to disable autoplay on desktop.
  NAME = 'load:media:youtube'
  URL = 'https://www.youtube.com/watch?v=QGfhS1hfTWw&autoplay=false'
  PLATFORM_SPECIFIC = True
  TAGS = [story_tags.EMERGING_MARKET]


class LoadDailymotionStory(_LoadingStory):
  # The side panel with related videos doesn't show on desktop due to
  # https://github.com/chromium/web-page-replay/issues/74.
  NAME = 'load:media:dailymotion'
  URL = (
      'https://www.dailymotion.com/video/x489k7d_street-performer-shows-off-slinky-skills_fun?autoplay=false')


class LoadGoogleImagesStory(_LoadingStory):
  NAME = 'load:media:google_images'
  URL = 'https://www.google.co.uk/search?tbm=isch&q=love'


class LoadSoundCloudStory(_LoadingStory):
  # No way to disable autoplay on desktop. Album artwork doesn't load due to
  # https://github.com/chromium/web-page-replay/issues/73.
  NAME = 'load:media:soundcloud'
  URL = 'https://soundcloud.com/lifeofdesiigner/desiigner-panda'


class Load9GagDesktopStory(_LoadingStory):
  NAME = 'load:media:9gag'
  URL = 'https://www.9gag.com/'
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY


class LoadImgurStory(_LoadingStory):
  NAME = 'load:media:imgur'
  URL = 'http://imgur.com/gallery/5UlBN'


class LoadFacebookPhotosMobileStory(_LoadingStory):
  """Load a page of rihanna's facebook with a photo."""
  NAME = 'load:media:facebook_photos'
  URL = (
      'https://m.facebook.com/rihanna/photos/a.207477806675.138795.10092511675/10153911739606676/?type=3&source=54&ref=page_internal')
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY
  TAGS = [story_tags.EMERGING_MARKET]


class LoadFacebookPhotosDesktopStory(_LoadingStory):
  """Load a page of rihanna's facebook with a photo."""
  NAME = 'load:media:facebook_photos'
  URL = (
      'https://www.facebook.com/rihanna/photos/a.207477806675.138795.10092511675/10153911739606676/?type=3&theater')
  # Recording currently does not work. The page gets stuck in the
  # theater viewer.
  SUPPORTED_PLATFORMS = platforms.NO_PLATFORMS


################################################################################
# Online tools (documents, emails, storage, ...).
################################################################################


class LoadDocsStory(_LoadingStory):
  """Load a typical google doc page."""
  NAME = 'load:tools:docs'
  URL = (
      'https://docs.google.com/document/d/1GvzDP-tTLmJ0myRhUAfTYWs3ZUFilUICg8psNHyccwQ/edit?usp=sharing')


class _LoadGmailBaseStory(_LoadingStory):
  NAME = 'load:tools:gmail'
  URL = 'https://mail.google.com/mail/'
  ABSTRACT_STORY = True

  def _Login(self, action_runner):
    google_login.LoginGoogleAccount(action_runner, 'googletest',
                                    self.credentials_path)

    # Navigating to https://mail.google.com immediately leads to an infinite
    # redirection loop due to a bug in WPR (see
    # https://github.com/chromium/web-page-replay/issues/70). We therefore first
    # navigate to a sub-URL to set up the session and hit the resulting
    # redirection loop. Afterwards, we can safely navigate to
    # https://mail.google.com.
    action_runner.tab.WaitForDocumentReadyStateToBeComplete()
    action_runner.Navigate(
        'https://mail.google.com/mail/mu/mp/872/trigger_redirection_loop')
    action_runner.tab.WaitForDocumentReadyStateToBeComplete()


class LoadGmailDesktopStory(_LoadGmailBaseStory):
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY

  def _DidLoadDocument(self, action_runner):
    # Wait until the UI loads.
    action_runner.WaitForJavaScriptCondition(
        'document.getElementById("loading").style.display === "none"')


class LoadGmailMobileStory(_LoadGmailBaseStory):
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

  def _DidLoadDocument(self, action_runner):
    # Wait until the UI loads.
    action_runner.WaitForElement('#apploadingdiv')
    action_runner.WaitForJavaScriptCondition(
        'document.getElementById("apploadingdiv").style.height === "0px"')

class LoadStackOverflowStory(_LoadingStory):
  """Load a typical question & answer page of stackoverflow.com"""
  NAME = 'load:tools:stackoverflow'
  URL = (
      'https://stackoverflow.com/questions/36827659/compiling-an-application-for-use-in-highly-radioactive-environments')


class LoadDropboxStory(_LoadingStory):
  NAME = 'load:tools:dropbox'
  URL = 'https://www.dropbox.com'

  def _Login(self, action_runner):
    dropbox_login.LoginAccount(action_runner, 'dropbox', self.credentials_path)


class LoadWeatherStory(_LoadingStory):
  NAME = 'load:tools:weather'
  URL = 'https://weather.com/en-GB/weather/today/l/USCA0286:1:US'
  TAGS = [story_tags.JAVASCRIPT_HEAVY]


class LoadDriveStory(_LoadingStory):
  NAME = 'load:tools:drive'
  URL = 'https://drive.google.com/drive/my-drive'
  TAGS = [story_tags.JAVASCRIPT_HEAVY]

  def _Login(self, action_runner):
    google_login.LoginGoogleAccount(action_runner, 'googletest',
                                    self.credentials_path)


################################################################################
# In-browser games (HTML5 and Flash).
################################################################################


class LoadBubblesStory(_LoadingStory):
  """Load "smarty bubbles" game on famobi.com"""
  NAME = 'load:games:bubbles'
  URL = (
      'https://games.cdn.famobi.com/html5games/s/smarty-bubbles/v010/?fg_domain=play.famobi.com&fg_uid=d8f24956-dc91-4902-9096-a46cb1353b6f&fg_pid=4638e320-4444-4514-81c4-d80a8c662371&fg_beat=620')

  def _DidLoadDocument(self, action_runner):
    # The #logo element is removed right before the main menu is displayed.
    action_runner.WaitForJavaScriptCondition(
        'document.getElementById("logo") === null')


class LoadLazorsStory(_LoadingStory):
  NAME = 'load:games:lazors'
  # Using "https://" hangs and shows "This site can't be reached".
  URL = 'http://www8.games.mobi/games/html5/lazors/lazors.html'


class LoadSpyChaseStory(_LoadingStory):
  NAME = 'load:games:spychase'
  # Using "https://" shows "Your connection is not private".
  URL = 'http://playstar.mobi/games/spychase/index.php'

  def _DidLoadDocument(self, action_runner):
    # The background of the game canvas is set when the "Tap screen to play"
    # caption is displayed.
    action_runner.WaitForJavaScriptCondition(
        'document.querySelector("#game canvas").style.background !== ""')


class LoadMiniclipStory(_LoadingStory):
  NAME = 'load:games:miniclip'
  # Using "https://" causes "404 Not Found" during WPR recording.
  URL = 'http://www.miniclip.com/games/en/'
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY  # Requires Flash.


class LoadAlphabettyStory(_LoadingStory):
  NAME = 'load:games:alphabetty'
  URL = 'https://king.com/play/alphabetty'
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY  # Requires Flash.

class LoadFlywheel0Story(_LoadingStory):
  NAME = 'flywheel1:em:medicinanet.com.br'
  URL = 'http://medicinanet.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheelCnnStory(_LoadingStory):
  NAME = 'flywheelcnn:em:cnn.com'
  url = 'http://www.cnn.com/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel990Story(_LoadingStory):
  NAME = 'flywheel1:em:crucbuzz.com'
  URL = 'http://m.cricbuzz.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel1Story(_LoadingStory):
  NAME = 'flywheel:em:sticknodes.com'
  URL = 'http://sticknodes.com/stickfigures/effects/?wpfb_list_page=2'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel2Story(_LoadingStory):
  NAME = 'flywheel:em:taxscan.in'
  URL = 'http://www.taxscan.in/cbdt-gets-highest-number-apa-requests-us/7226/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel3Story(_LoadingStory):
  NAME = 'flywheel:em:klikbca.com'
  URL = 'http://klikbca.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel4Story(_LoadingStory):
  NAME = 'flywheel:em:minhavida.com.br'
  URL = 'http://minhavida.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel5Story(_LoadingStory):
  NAME = 'flywheel1:em:www.sarkariresult.com'
  URL = 'http://www.sarkariresult.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel6Story(_LoadingStory):
  NAME = 'flywheel:em:guiadejardineria.com'
  URL = 'http://www.guiadejardineria.com/los-mejores-arboles-para-bonsais/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel7Story(_LoadingStory):
  NAME = 'flywheel:em:tudocelular.com'
  URL = 'http://tudocelular.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel8Story(_LoadingStory):
  NAME = 'flywheel:em:chile365.cl'
  URL = 'http://www.chile365.cl/es-rios-chilenos.php'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel9Story(_LoadingStory):
  NAME = 'flywheel:em:upcomingrailwayrecruitment.in'
  URL = 'http://upcomingrailwayrecruitment.in/rrb-allahabad-result-2017-2018-www-rrbald-gov-in/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel10Story(_LoadingStory):
  NAME = 'flywheel:em:m.torontosun.com'
  URL = 'http://m.torontosun.com/2017/05/03/furey-trudeau-tells-world-media-alternative-facts-about-canadas-muslim-related-policies'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel11Story(_LoadingStory):
  NAME = 'flywheel:em:mundodvd.com'
  URL = 'http://www.mundodvd.com/cinefilia/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel12Story(_LoadingStory):
  NAME = 'flywheel:em:buscape.com.br'
  URL = 'http://buscape.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel13Story(_LoadingStory):
  NAME = 'flywheel:em:newsit.gr'
  URL = 'http://www.newsit.gr/topikes-eidhseis/Esvise-26xronos-ston-Volo-Ton-vrike-nekro-i-fili-toy/725742'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel14Story(_LoadingStory):
  NAME = 'flywheel:em:onepi.sp.mbga.jp'
  URL = 'http://onepi.sp.mbga.jp/_onepi_event232_wlsg_rw_l'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel15Story(_LoadingStory):
  NAME = 'flywheel:em:minhateca.com.br'
  URL = 'http://minhateca.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel16Story(_LoadingStory):
  NAME = 'flywheel:em:br39.ru'
  URL = 'http://br39.ru/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel17Story(_LoadingStory):
  NAME = 'flywheel:em:kapanlagi.com'
  URL = 'http://kapanlagi.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel18Story(_LoadingStory):
  NAME = 'flywheel:em:todosinstrumentosmusicais.com.br'
  URL = 'http://www.todosinstrumentosmusicais.com.br/page/3'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel19Story(_LoadingStory):
  NAME = 'flywheel:em:about.com'
  URL = 'http://about.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel20Story(_LoadingStory):
  NAME = 'flywheel:em:casaevideo.com.br'
  URL = 'http://www.casaevideo.com.br/cx-amplific-bluet-usb-aux-mondial-mco04/p'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel21Story(_LoadingStory):
  NAME = 'flywheel:em:netshoes.com.br'
  URL = 'http://www.netshoes.com.br/relogios/feminino?No=90'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel22Story(_LoadingStory):
  NAME = 'flywheel:em:songspk.onl'
  URL = 'http://songspk.onl'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel23Story(_LoadingStory):
  NAME = 'flywheel:em:nutrangcuoivn.com'
  URL = 'http://nutrangcuoivn.com/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel24Story(_LoadingStory):
  NAME = 'flywheel:em:caixa.gov.br'
  URL = 'http://caixa.gov.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel25Story(_LoadingStory):
  NAME = 'flywheel:em:brasilescola.com'
  URL = 'http://brasilescola.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel26Story(_LoadingStory):
  NAME = 'flywheel:em:aliexpress.com'
  URL = 'http://aliexpress.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel27Story(_LoadingStory):
  NAME = 'flywheel:em:r7.com'
  URL = 'http://r7.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel28Story(_LoadingStory):
  NAME = 'flywheel:em:apontador.com.br'
  URL = 'http://apontador.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel29Story(_LoadingStory):
  NAME = 'flywheel:em:jne.co.id'
  URL = 'http://jne.co.id'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel30Story(_LoadingStory):
  NAME = 'flywheel:em:rediff.com'
  URL = 'http://rediff.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel31Story(_LoadingStory):
  NAME = 'flywheel:em:piercingytatuajes.com'
  URL = 'http://piercingytatuajes.com/tatuajes-de-buhos-lechuzas'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel32Story(_LoadingStory):
  NAME = 'flywheel:em:u1sokuhou.ldblog.jp'
  URL = 'http://u1sokuhou.ldblog.jp/archives/50499003.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel33Story(_LoadingStory):
  NAME = 'flywheel:em:uyeshare.com'
  URL = 'http://uyeshare.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel34Story(_LoadingStory):
  NAME = 'flywheel:em:suamusica.com.br'
  URL = 'http://suamusica.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel35Story(_LoadingStory):
  NAME = 'flywheel:em:mercadolivre.com.br'
  URL = 'http://mercadolivre.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel36Story(_LoadingStory):
  NAME = 'flywheel:em:wuxiaworld.com'
  URL = 'http://www.wuxiaworld.com/wmw-index/wmw-chapter-611/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel37Story(_LoadingStory):
  NAME = 'flywheel:em:infoescola.com'
  URL = 'http://infoescola.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel38Story(_LoadingStory):
  NAME = 'flywheel:em:iplt20.com'
  URL = 'http://iplt20.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel39Story(_LoadingStory):
  NAME = 'flywheel:em:ammonnews.net'
  URL = 'http://www.ammonnews.net/article/312611'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel40Story(_LoadingStory):
  NAME = 'flywheel:em:slideshare.net'
  URL = 'http://slideshare.net'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel41Story(_LoadingStory):
  NAME = 'flywheel:em:yoamoloszapatos.com'
  URL = 'http://www.yoamoloszapatos.com/lifestyle/los-signos-zodiacales-excelentes-la-cama-uno'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel42Story(_LoadingStory):
  NAME = 'flywheel:em:fast2car.com'
  URL = 'http://www.fast2car.com/car.php?brand_id=57&model_id=357&body_id=987'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel43Story(_LoadingStory):
  NAME = 'flywheel:em:m.moneycontrol.com'
  URL = 'http://m.moneycontrol.com/stock-message-forum/layla-textile/comments/509896?pgno=6&que=LT'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel44Story(_LoadingStory):
  NAME = 'flywheel:em:olx.co.id'
  URL = 'http://olx.co.id'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel45Story(_LoadingStory):
  NAME = 'flywheel:em:m.olx.com.br'
  URL = 'http://m.olx.com.br/busca?cg=2040&o=19&w=3'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel46Story(_LoadingStory):
  NAME = 'flywheel:em:m.24h.com.vn'
  URL = 'http://m.24h.com.vn/giai-tri/co-gai-an-keo-thay-doi-ngo-ngang-noi-bat-giua-dan-guong-mat-than-quen-c731a872428.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel47Story(_LoadingStory):
  NAME = 'flywheel:em:dicionarioinformal.com.br'
  URL = 'http://dicionarioinformal.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel48Story(_LoadingStory):
  NAME = 'flywheel:em:belasmensagens.com.br'
  URL = 'http://belasmensagens.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel49Story(_LoadingStory):
  NAME = 'flywheel:em:mg.gov.br'
  URL = 'http://mg.gov.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel50Story(_LoadingStory):
  NAME = 'flywheel:em:askmen.com'
  URL = 'http://www.askmen.com/top_10/dating/top-10-proven-signs-shes-interested_7.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel51Story(_LoadingStory):
  NAME = 'flywheel:em:jabong.com'
  URL = 'http://jabong.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel52Story(_LoadingStory):
  NAME = 'flywheel:em:republika.co.id'
  URL = 'http://republika.co.id'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel53Story(_LoadingStory):
  NAME = 'flywheel:em:crecerfeliz.es'
  URL = 'http://www.crecerfeliz.es/El-bebe/Buenos-cuidados/Sentar-al-bebe/Tu-ayuda-es-fundamental'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel54Story(_LoadingStory):
  NAME = 'flywheel:em:vehiculo.mercadolibre.com.ar'
  URL = 'http://vehiculo.mercadolibre.com.ar/MLA-663772252-chocado-renault-sandero-stepwey-2012-airbags-sanos-_JM'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel55Story(_LoadingStory):
  NAME = 'flywheel:em:bursalagu.com'
  URL = 'http://bursalagu.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel56Story(_LoadingStory):
  NAME = 'flywheel:em:sattamatka.mobi'
  URL = 'http://sattamatka.mobi'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel57Story(_LoadingStory):
  NAME = 'flywheel:em:ojogodobicho.com'
  URL = 'http://ojogodobicho.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel58Story(_LoadingStory):
  NAME = 'flywheel:em:cdn4.crichd.info'
  URL = 'http://cdn4.crichd.info/bein-sports-2-france-live-streaming?v=m'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel59Story(_LoadingStory):
  NAME = 'flywheel:em:habeebtv.com'
  URL = 'http://habeebtv.com/tvmobile/india/view.php?cat_id=2&ch_id=1377'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel60Story(_LoadingStory):
  NAME = 'flywheel:em:jadwal21.com'
  URL = 'http://jadwal21.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel61Story(_LoadingStory):
  NAME = 'flywheel:em:cumhuriyet.com.tr'
  URL = 'http://www.cumhuriyet.com.tr/m/haber/siyaset/737382/_Once_yanlis_dugmeyi_sokelim_.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel62Story(_LoadingStory):
  NAME = 'flywheel:em:shopclues.com'
  URL = 'http://shopclues.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel63Story(_LoadingStory):
  NAME = 'flywheel:em:kompas.com'
  URL = 'http://kompas.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel64Story(_LoadingStory):
  NAME = 'flywheel:em:cricbuzz.com'
  URL = 'http://cricbuzz.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel65Story(_LoadingStory):
  NAME = 'flywheel:em:subtitlecity.net'
  URL = 'http://subtitlecity.net/%D8%AF%D8%A7%D9%86%D9%84%D9%88%D8%AF-%D8%B2%DB%8C%D8%B1%D9%86%D9%88%DB%8C%D8%B3-%D9%81%D8%A7%D8%B1%D8%B3%DB%8C-%D9%81%DB%8C%D9%84%D9%85-fifty-shades-darker-2017/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel66Story(_LoadingStory):
  NAME = 'flywheel:em:camim.com.br'
  URL = 'http://www.camim.com.br/financeiro'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel67Story(_LoadingStory):
  NAME = 'flywheel:em:etimologias.dechile.net'
  URL = 'http://etimologias.dechile.net/?ence.falo'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel68Story(_LoadingStory):
  NAME = 'flywheel:em:purepeople.com.br'
  URL = 'http://purepeople.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel69Story(_LoadingStory):
  NAME = 'flywheel:em:okezone.com'
  URL = 'http://okezone.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel70Story(_LoadingStory):
  NAME = 'flywheel:em:m.mangatown.com'
  URL = 'http://m.mangatown.com/manga/parallel_paradise/c002/21.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel71Story(_LoadingStory):
  NAME = 'flywheel:em:hariangadget.com'
  URL = 'http://hariangadget.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel72Story(_LoadingStory):
  NAME = 'flywheel:em:goal.com'
  URL = 'http://goal.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel73Story(_LoadingStory):
  NAME = 'flywheel:em:lancenet.com.br'
  URL = 'http://lancenet.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel74Story(_LoadingStory):
  NAME = 'flywheel:em:dressto.com.br'
  URL = 'http://dressto.com.br/dress-to-shop/dress-code'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel75Story(_LoadingStory):
  NAME = 'flywheel:em:vegrecipesofindia.com'
  URL = 'http://vegrecipesofindia.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel76Story(_LoadingStory):
  NAME = 'flywheel:em:espncricinfo.com'
  URL = 'http://espncricinfo.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel77Story(_LoadingStory):
  NAME = 'flywheel:em:jagran.com'
  URL = 'http://jagran.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel78Story(_LoadingStory):
  NAME = 'flywheel:em:amazon.in'
  URL = 'http://www.amazon.in/Autocop-Car-Gear-Lock/dp/B00YE6N52G'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel79Story(_LoadingStory):
  NAME = 'flywheel:em:indiamart.com'
  URL = 'http://indiamart.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel80Story(_LoadingStory):
  NAME = 'flywheel:em:m.olx.com.br-2'
  URL = 'http://m.olx.com.br/anuncio?ad_id=326451309'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel81Story(_LoadingStory):
  NAME = 'flywheel:em:m.accuweather.com'
  URL = 'http://m.accuweather.com/en/za/pretoria/305449/current-weather/305449'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel82Story(_LoadingStory):
  NAME = 'flywheel:em:lista.mercadolivre.com.br'
  URL = 'http://lista.mercadolivre.com.br/sala-estar/sofas/sofa-retratil/sofa-retratil_Desde_21'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel83Story(_LoadingStory):
  NAME = 'flywheel:em:arabalar.com.tr'
  URL = 'http://www.arabalar.com.tr/land-rover/range-rover-sport'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel84Story(_LoadingStory):
  NAME = 'flywheel:em:ndtv.com'
  URL = 'http://ndtv.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel85Story(_LoadingStory):
  NAME = 'flywheel:em:google.com'
  URL = 'http://google.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel86Story(_LoadingStory):
  NAME = 'flywheel:em:lazada.co.id'
  URL = 'http://lazada.co.id'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel87Story(_LoadingStory):
  NAME = 'flywheel:em:bidanku.com'
  URL = 'http://bidanku.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel88Story(_LoadingStory):
  NAME = 'flywheel:em:sapo.pt'
  URL = 'http://sapo.pt'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel89Story(_LoadingStory):
  NAME = 'flywheel:em:medtronic.ru'
  URL = 'http://www.medtronic.ru/your-health/diabetes/device/insulin-pump/what-is-it/index.htm'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel90Story(_LoadingStory):
  NAME = 'flywheel:em:vix.com'
  URL = 'http://www.vix.com/es/ciudadanos/180967/su-padre-la-ataco-con-acido-porque-se-entero-de-su-mas-cruel-secreto-era-traficante-de-ninas?utm_term=Autofeed&utm_campaign=Echobox&utm_medium=VixCiencia&utm_source=Facebook#link_time=1494697240'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel91Story(_LoadingStory):
  NAME = 'flywheel:em:musica.com.br'
  URL = 'http://musica.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel92Story(_LoadingStory):
  NAME = 'flywheel:em:magiclesson.ru'
  URL = 'http://magiclesson.ru/category/fokusy-s-kartami/page/2'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel93Story(_LoadingStory):
  NAME = 'flywheel:em:sportstats.com'
  URL = 'http://www.sportstats.com/baseball/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel94Story(_LoadingStory):
  NAME = 'flywheel:em:computacion.mercadolibre.com.ar'
  URL = 'http://computacion.mercadolibre.com.ar/tablets-accesorios/_Deal_hot-sale-electronica'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel95Story(_LoadingStory):
  NAME = 'flywheel:em:americanas.com.br'
  URL = 'http://americanas.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel96Story(_LoadingStory):
  NAME = 'flywheel:em:sies.saude.gov.br'
  URL = 'http://sies.saude.gov.br/senha.asp'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel97Story(_LoadingStory):
  NAME = 'flywheel:em:oniyomech.livedoor.biz'
  URL = 'http://oniyomech.livedoor.biz/archives/49987690.html?p=2'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel98Story(_LoadingStory):
  NAME = 'flywheel:em:kodepos.posindonesia.co.id'
  URL = 'http://kodepos.posindonesia.co.id/kodeposalamatindonesialist.php?cmd=search&t=kodeposalamatindonesia&z_Propinsi=%3D&x_Propinsi=33&psearch=&psearchtype='
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel99Story(_LoadingStory):
  NAME = 'flywheel:em:ibuhamil.com'
  URL = 'http://ibuhamil.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel100Story(_LoadingStory):
  NAME = 'flywheel:em:savefrom.net'
  URL = 'http://savefrom.net'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel101Story(_LoadingStory):
  NAME = 'flywheel:em:m.etrain.info'
  URL = 'http://m.etrain.info/in-CRS?TRAIN=12705'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel102Story(_LoadingStory):
  NAME = 'flywheel:em:techtudo.com.br'
  URL = 'http://techtudo.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel103Story(_LoadingStory):
  NAME = 'flywheel:em:bonprix.pl'
  URL = 'http://www.bonprix.pl/kategoria/6/sale/?utm_source=o2&utm_medium=e-mail&utm_campaign=o2_ext'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel104Story(_LoadingStory):
  NAME = 'flywheel:em:guiamais.com.br'
  URL = 'http://guiamais.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel105Story(_LoadingStory):
  NAME = 'flywheel:em:timesofindia.com'
  URL = 'http://timesofindia.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel106Story(_LoadingStory):
  NAME = 'flywheel:em:icarros.com.br'
  URL = 'http://icarros.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel107Story(_LoadingStory):
  NAME = 'flywheel:em:olx.com.br'
  URL = 'http://olx.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel108Story(_LoadingStory):
  NAME = 'flywheel:em:beygir.com'
  URL = 'http://www.beygir.com/servisler/sorgulama/Atsorgu/AtKosu.asp?ref=77176'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel109Story(_LoadingStory):
  NAME = 'flywheel:em:tamildownloads.net'
  URL = 'http://tamildownloads.net/tamilmp3/index.php?dir=Devotional%20Songs/Ayyappan%20Songs/Pallikkatu%20Sabarimalaikku&p=1&sort=1'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel110Story(_LoadingStory):
  NAME = 'flywheel:em:m.eltiempo.com'
  URL = 'http://m.eltiempo.com/cultura/gente/cristian-castro-pide-matrimonio-a-su-novia-en-mitad-de-un-concierto-mexico-85376'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel111Story(_LoadingStory):
  NAME = 'flywheel:em:newstabarjal.com'
  URL = 'http://www.newstabarjal.com/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel112Story(_LoadingStory):
  NAME = 'flywheel:em:india-forums.com'
  URL = 'http://india-forums.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel113Story(_LoadingStory):
  NAME = 'flywheel:em:raagtune.com'
  URL = 'http://raagtune.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel114Story(_LoadingStory):
  NAME = 'flywheel:em:sindonews.com'
  URL = 'http://sindonews.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel115Story(_LoadingStory):
  NAME = 'flywheel:em:azlyrics.com'
  URL = 'http://azlyrics.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel116Story(_LoadingStory):
  NAME = 'flywheel:em:otosia.com'
  URL = 'http://otosia.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel117Story(_LoadingStory):
  NAME = 'flywheel:em:gsmarena.com'
  URL = 'http://gsmarena.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel118Story(_LoadingStory):
  NAME = 'flywheel:em:metrotvnews.com'
  URL = 'http://metrotvnews.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel119Story(_LoadingStory):
  NAME = 'flywheel:em:espncricinfo.com-2'
  URL = 'http://www.espncricinfo.com/indian-premier-league-2017/engine/match/1082639.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel120Story(_LoadingStory):
  NAME = 'flywheel:em:samsung.com'
  URL = 'http://samsung.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel121Story(_LoadingStory):
  NAME = 'flywheel:em:correios.com.br'
  URL = 'http://correios.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel122Story(_LoadingStory):
  NAME = 'flywheel:em:oneindia.com'
  URL = 'http://oneindia.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel123Story(_LoadingStory):
  NAME = 'flywheel:em:wikia.com'
  URL = 'http://wikia.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel124Story(_LoadingStory):
  NAME = 'flywheel:em:babycenter.com'
  URL = 'http://babycenter.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel125Story(_LoadingStory):
  NAME = 'flywheel:em:mangaku.web.id'
  URL = 'http://mangaku.web.id'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel126Story(_LoadingStory):
  NAME = 'flywheel:em:isepsantafe.edu.ar'
  URL = 'http://www.isepsantafe.edu.ar/2016aux/moodle/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel127Story(_LoadingStory):
  NAME = 'flywheel:em:freejobalert.com'
  URL = 'http://freejobalert.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel128Story(_LoadingStory):
  NAME = 'flywheel:em:kboing.com.br'
  URL = 'http://kboing.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel129Story(_LoadingStory):
  NAME = 'flywheel:em:amazon.in-2'
  URL = 'http://amazon.in'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel130Story(_LoadingStory):
  NAME = 'flywheel:em:webmusic.cc'
  URL = 'http://webmusic.cc/bengali_music.php?id=1909'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel131Story(_LoadingStory):
  NAME = 'flywheel:em:sabah.com.tr'
  URL = 'http://www.sabah.com.tr/gundem/2017/05/12/chp-bolu-il-baskan-yardimcisi-fetoden-tutuklandi'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel132Story(_LoadingStory):
  NAME = 'flywheel:em:tugo.com.vn'
  URL = 'http://www.tugo.com.vn/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel133Story(_LoadingStory):
  NAME = 'flywheel:em:rj.gov.br'
  URL = 'http://rj.gov.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel134Story(_LoadingStory):
  NAME = 'flywheel:em:webmusic.in'
  URL = 'http://webmusic.in'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel135Story(_LoadingStory):
  NAME = 'flywheel:em:portal.orgcard.com.br'
  URL = 'http://portal.orgcard.com.br/sindplus/cw_rel_login_cpf_param.php'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel136Story(_LoadingStory):
  NAME = 'flywheel:em:claro.com.co'
  URL = 'http://www.claro.com.co/personas/servicios/claro-club-login/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel137Story(_LoadingStory):
  NAME = 'flywheel:em:steves-digicams.com'
  URL = 'http://www.steves-digicams.com/knowledge-center/how-tos/digital-camera-operation/the-difference-between-a-169-aspect-ratio-and-43-aspect-ratio.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel138Story(_LoadingStory):
  NAME = 'flywheel:em:hurriyet.com.tr'
  URL = 'http://www.hurriyet.com.tr/otomobilinden-kokain-alanlarin-listesi-cikti-40446353'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel139Story(_LoadingStory):
  NAME = 'flywheel:em:imdb.com'
  URL = 'http://imdb.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel140Story(_LoadingStory):
  NAME = 'flywheel:em:androidfreeware.net'
  URL = 'http://www.androidfreeware.net/download-tubemate-1-5.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel141Story(_LoadingStory):
  NAME = 'flywheel:em:m.olx.com.br-3'
  URL = 'http://m.olx.com.br/busca?ca=21_s&cg=5060&q=milheiro&w=6'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel142Story(_LoadingStory):
  NAME = 'flywheel:em:myntra.com'
  URL = 'http://myntra.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel143Story(_LoadingStory):
  NAME = 'flywheel:em:india.com'
  URL = 'http://india.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel144Story(_LoadingStory):
  NAME = 'flywheel:em:ivoirebusiness.net'
  URL = 'http://www.ivoirebusiness.net/articles/scandale-adoption-en-c%C3%B4te-d%E2%80%99ivoire-d%E2%80%99un-projet-de-loi-sur-l%E2%80%99emprisonnement-des-journalistes'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel145Story(_LoadingStory):
  NAME = 'flywheel:em:runningstatus.in'
  URL = 'http://runningstatus.in'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel146Story(_LoadingStory):
  NAME = 'flywheel:em:liputan6.com'
  URL = 'http://liputan6.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel147Story(_LoadingStory):
  NAME = 'flywheel:em:hdrezka.me'
  URL = 'http://hdrezka.me/series/comedy/1863-gorod-hischnic.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel148Story(_LoadingStory):
  NAME = 'flywheel:em:4shared.com'
  URL = 'http://4shared.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel149Story(_LoadingStory):
  NAME = 'flywheel:em:wapka.mobi'
  URL = 'http://wapka.mobi'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel150Story(_LoadingStory):
  NAME = 'flywheel:em:infinitydesign.in.th'
  URL = 'http://www.infinitydesign.in.th/%E0%B8%87%E0%B9%88%E0%B8%B2%E0%B8%A2%E0%B8%99%E0%B8%B4%E0%B8%94%E0%B9%80%E0%B8%94%E0%B8%B5%E0%B8%A2%E0%B8%A7-6-%E0%B8%A7%E0%B8%B4%E0%B8%98%E0%B8%B5%E0%B9%84%E0%B8%A5%E0%B9%88%E0%B8%88%E0%B8%B4%E0%B9%89%E0%B8%87%E0%B8%88%E0%B8%81%E0%B9%83%E0%B8%AB%E0%B9%89%E0%B9%84%E0%B8%9B%E0%B9%84%E0%B8%81%E0%B8%A5%E0%B8%9A%E0%B9%89%E0%B8%B2%E0%B8%99/50644'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel151Story(_LoadingStory):
  NAME = 'flywheel:em:botanical-online.com'
  URL = 'http://www.botanical-online.com/animales/piojillo_aves.htm'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel152Story(_LoadingStory):
  NAME = 'flywheel:em:dailymedicalinfo.com'
  URL = 'http://www.dailymedicalinfo.com/view-article/%d9%84%d8%a8%d9%86%d8%a7%d8%a1-%d8%b9%d8%b6%d9%84%d8%a7%d8%aa-%d8%ac%d8%b3%d9%85%d9%83-%d8%aa%d9%86%d8%a7%d9%88%d9%84-%d8%aa%d9%84%d9%83-%d8%a7%d9%84%d8%a3%d8%b7%d8%b9%d9%85%d8%a9/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel153Story(_LoadingStory):
  NAME = 'flywheel:em:sattamatkano1.net'
  URL = 'http://sattamatkano1.net'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel154Story(_LoadingStory):
  NAME = 'flywheel:em:money.rbc.ru'
  URL = 'http://money.rbc.ru/news/58e3d10e9a794774d4b7759b?from=money'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel155Story(_LoadingStory):
  NAME = 'flywheel:em:tecmundo.com.br'
  URL = 'http://tecmundo.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel156Story(_LoadingStory):
  NAME = 'flywheel:em:vivo.com.br'
  URL = 'http://vivo.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel157Story(_LoadingStory):
  NAME = 'flywheel:em:m.olx.com.br-4'
  URL = 'http://m.olx.com.br/busca?cg=2020&o=2&vb=64&vm=9&w=3'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel158Story(_LoadingStory):
  NAME = 'flywheel:em:adorocinema.com'
  URL = 'http://adorocinema.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel159Story(_LoadingStory):
  NAME = 'flywheel:em:mec.gov.br'
  URL = 'http://mec.gov.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel160Story(_LoadingStory):
  NAME = 'flywheel:em:indonetwork.co.id'
  URL = 'http://indonetwork.co.id'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel161Story(_LoadingStory):
  NAME = 'flywheel:em:denizevdenevenakliyat.com'
  URL = 'http://www.denizevdenevenakliyat.com/konya-otobus-tarifeleri/125_loras-telafer-karaman-yolu-haftalik-otobus-tarifesi.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel162Story(_LoadingStory):
  NAME = 'flywheel:em:pagalworld.com'
  URL = 'http://pagalworld.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel163Story(_LoadingStory):
  NAME = 'flywheel:em:telkomsel.com'
  URL = 'http://telkomsel.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel164Story(_LoadingStory):
  NAME = 'flywheel:em:m.accuweather.com-2'
  URL = 'http://m.accuweather.com/es/mx/santa-fe/237611/daily-weather-forecast/237611?day=2&unit=c&partner=samand'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel165Story(_LoadingStory):
  NAME = 'flywheel:em:hotstar.com'
  URL = 'http://www.hotstar.com/1000174918'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel166Story(_LoadingStory):
  NAME = 'flywheel:em:music.com'
  URL = 'http://music.com/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel167Story(_LoadingStory):
  NAME = 'flywheel:em:youtube-mp3.org'
  URL = 'http://youtube-mp3.org'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel168Story(_LoadingStory):
  NAME = 'flywheel:em:bankmandiri.co.id'
  URL = 'http://bankmandiri.co.id'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel169Story(_LoadingStory):
  NAME = 'flywheel:em:baixaki.com.br'
  URL = 'http://baixaki.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel170Story(_LoadingStory):
  NAME = 'flywheel:em:cuidadoinfantil.net'
  URL = 'http://cuidadoinfantil.net/tabla-de-imc-para-ninos-de-5-a-19-anos.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel171Story(_LoadingStory):
  NAME = 'flywheel:em:articulo.mercadolibre.com.ar'
  URL = 'http://articulo.mercadolibre.com.ar/MLA-661265010-botas-adidas-cacity-mid-_JM'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel172Story(_LoadingStory):
  NAME = 'flywheel:em:live.com'
  URL = 'http://live.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel173Story(_LoadingStory):
  NAME = 'flywheel:em:indeed.com.br'
  URL = 'http://indeed.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel174Story(_LoadingStory):
  NAME = 'flywheel:em:loja.semparar.com.br'
  URL = 'http://loja.semparar.com.br/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel175Story(_LoadingStory):
  NAME = 'flywheel:em:wikihow.com'
  URL = 'http://wikihow.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel176Story(_LoadingStory):
  NAME = 'flywheel:em:mp3fil.info'
  URL = 'http://mp3fil.info'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel177Story(_LoadingStory):
  NAME = 'flywheel:em:tricsehat.com'
  URL = 'http://www.tricsehat.com/2017/04/bayi-batuk-pilek-tak-butuh-obat-kimia.html?spref=fb&m=1'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel178Story(_LoadingStory):
  NAME = 'flywheel:em:dailymotion.com'
  URL = 'http://dailymotion.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel179Story(_LoadingStory):
  NAME = 'flywheel:em:tudogostoso.com.br'
  URL = 'http://tudogostoso.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel180Story(_LoadingStory):
  NAME = 'flywheel:em:thehindu.com'
  URL = 'http://thehindu.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel181Story(_LoadingStory):
  NAME = 'flywheel:em:produto.mercadolivre.com.br'
  URL = 'http://produto.mercadolivre.com.br/MLB-789784982-sela-vaquejada-profissional-frete-gratis-_JM'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel182Story(_LoadingStory):
  NAME = 'flywheel:em:indeed.co.in'
  URL = 'http://indeed.co.in'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel183Story(_LoadingStory):
  NAME = 'flywheel:em:india.com-2'
  URL = 'http://www.india.com/showbiz/ranveer-singh-and-prabhas-to-fight-it-out-to-star-in-baahubali-director-ss-rajamoulis-next-2115057/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel184Story(_LoadingStory):
  NAME = 'flywheel:em:bolsademulher.com'
  URL = 'http://bolsademulher.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel185Story(_LoadingStory):
  NAME = 'flywheel:em:rotinadebelezamarykay.com.br'
  URL = 'http://www.rotinadebelezamarykay.com.br/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel186Story(_LoadingStory):
  NAME = 'flywheel:em:vodafone.in'
  URL = 'http://vodafone.in'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel187Story(_LoadingStory):
  NAME = 'flywheel:em:indianexpress.com'
  URL = 'http://indianexpress.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel188Story(_LoadingStory):
  NAME = 'flywheel:em:airtel.in'
  URL = 'http://airtel.in'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel189Story(_LoadingStory):
  NAME = 'flywheel:em:entertain.teenee.com'
  URL = 'http://entertain.teenee.com/gossip/158674.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel190Story(_LoadingStory):
  NAME = 'flywheel:em:uchim.org'
  URL = 'http://uchim.org/gdz/po-algebre-8-klass-mordkovich/1435'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel191Story(_LoadingStory):
  NAME = 'flywheel:em:wmrfast.com'
  URL = 'http://wmrfast.com/serfing.php'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel192Story(_LoadingStory):
  NAME = 'flywheel:em:mp3terbaru.info'
  URL = 'http://mp3terbaru.info'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel193Story(_LoadingStory):
  NAME = 'flywheel:em:piterzavtra.ru'
  URL = 'http://piterzavtra.ru/prazdnik-cveteniya-sakury/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel194Story(_LoadingStory):
  NAME = 'flywheel:em:deperu.com'
  URL = 'http://www.deperu.com/abc/diferencias-significado/4177/diferencia-entre-estrategia-y-tactica'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel195Story(_LoadingStory):
  NAME = 'flywheel:em:birthbecomesher.com'
  URL = 'http://www.birthbecomesher.com/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel196Story(_LoadingStory):
  NAME = 'flywheel:em:clicrbs.com.br'
  URL = 'http://clicrbs.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel197Story(_LoadingStory):
  NAME = 'flywheel:em:notefood.ru'
  URL = 'http://notefood.ru/retsepty-blyud/vtory-e-blyuda/tefteli-iz-kurinogo-farsha.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel198Story(_LoadingStory):
  NAME = 'flywheel:em:m.mangatown.com-2'
  URL = 'http://m.mangatown.com/manga/days/c109/20.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel199Story(_LoadingStory):
  NAME = 'flywheel:em:ask.fm'
  URL = 'http://ask.fm'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel200Story(_LoadingStory):
  NAME = 'flywheel:em:lagaceta.com.ar'
  URL = 'http://www.lagaceta.com.ar/nota/729881/actualidad/urna-funeraria-podria-ser-puente-tafi-viejo-tafi-valle.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel201Story(_LoadingStory):
  NAME = 'flywheel:em:th.coinmill.com'
  URL = 'http://th.coinmill.com/PLN_THB.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel202Story(_LoadingStory):
  NAME = 'flywheel:em:ncode.syosetu.com'
  URL = 'http://ncode.syosetu.com/n8802bq/56/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel203Story(_LoadingStory):
  NAME = 'flywheel:em:ar.prvademecum.com'
  URL = 'http://ar.prvademecum.com/producto.php?producto=1344'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel204Story(_LoadingStory):
  NAME = 'flywheel:em:xossip.com'
  URL = 'http://xossip.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel205Story(_LoadingStory):
  NAME = 'flywheel:em:sulekha.com'
  URL = 'http://sulekha.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel206Story(_LoadingStory):
  NAME = 'flywheel:em:m.xemvtv.net'
  URL = 'http://m.xemvtv.net/xem-phim/nu-tu-nhan-tap-2/5WU0OZ.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel207Story(_LoadingStory):
  NAME = 'flywheel:em:adnradio.cl'
  URL = 'http://www.adnradio.cl/noticias/politica/cadem-beatriz-sanchez-sigue-al-alza-y-guillier-baja-al-15/20170508/nota/3457039.aspx'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel208Story(_LoadingStory):
  NAME = 'flywheel:em:m.mangahere.co'
  URL = 'http://m.mangahere.co/manga/choujin_koukousei_tachi_wa_isekai_demo_yoyuu_de_ikinuku_you_desu/c005/3.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel209Story(_LoadingStory):
  NAME = 'flywheel:em:kompasiana.com'
  URL = 'http://kompasiana.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel210Story(_LoadingStory):
  NAME = 'flywheel:em:mangalam.com'
  URL = 'http://www.mangalam.com/news/detail/105226-celebrity.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel211Story(_LoadingStory):
  NAME = 'flywheel:em:quikr.com'
  URL = 'http://quikr.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel212Story(_LoadingStory):
  NAME = 'flywheel:em:jpnn.com'
  URL = 'http://jpnn.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel213Story(_LoadingStory):
  NAME = 'flywheel:em:stafaband.info'
  URL = 'http://stafaband.info'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel214Story(_LoadingStory):
  NAME = 'flywheel:em:metrolyrics.com'
  URL = 'http://metrolyrics.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel215Story(_LoadingStory):
  NAME = 'flywheel:em:hdfcbank.com'
  URL = 'http://hdfcbank.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel216Story(_LoadingStory):
  NAME = 'flywheel:em:vemale.com'
  URL = 'http://vemale.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel217Story(_LoadingStory):
  NAME = 'flywheel:em:bola.net'
  URL = 'http://bola.net'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel218Story(_LoadingStory):
  NAME = 'flywheel:em:abril.com.br'
  URL = 'http://abril.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel219Story(_LoadingStory):
  NAME = 'flywheel:em:tribunnews.com'
  URL = 'http://tribunnews.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel220Story(_LoadingStory):
  NAME = 'flywheel:em:reclameaqui.com.br'
  URL = 'http://reclameaqui.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel221Story(_LoadingStory):
  NAME = 'flywheel:em:21cineplex.com'
  URL = 'http://21cineplex.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel222Story(_LoadingStory):
  NAME = 'flywheel:em:globo.com'
  URL = 'http://globo.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel223Story(_LoadingStory):
  NAME = 'flywheel:em:dgft.delhi.nic.in'
  URL = 'http://dgft.delhi.nic.in/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel224Story(_LoadingStory):
  NAME = 'flywheel:em:chordfrenzy.com'
  URL = 'http://chordfrenzy.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel225Story(_LoadingStory):
  NAME = 'flywheel:em:jusbrasil.com.br'
  URL = 'http://jusbrasil.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel226Story(_LoadingStory):
  NAME = 'flywheel:em:webmd.com'
  URL = 'http://webmd.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel227Story(_LoadingStory):
  NAME = 'flywheel:em:articulo.mercadolibre.com.ar-2'
  URL = 'http://articulo.mercadolibre.com.ar/MLA-632421264-mesa-ratona-de-centro-diseno-moderno-exclusivo-_JM'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel228Story(_LoadingStory):
  NAME = 'flywheel:em:climatempo.com.br'
  URL = 'http://climatempo.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel229Story(_LoadingStory):
  NAME = 'flywheel:em:rockinrio.com'
  URL = 'http://rockinrio.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel230Story(_LoadingStory):
  NAME = 'flywheel:em:estilodominicano.com'
  URL = 'http://estilodominicano.com/2017/05/el-dia-hoy-esta-noche/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel231Story(_LoadingStory):
  NAME = 'flywheel:em:moneycontrol.com'
  URL = 'http://moneycontrol.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel232Story(_LoadingStory):
  NAME = 'flywheel:em:dota2.vpgame.com'
  URL = 'http://dota2.vpgame.com/user/my.html'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel233Story(_LoadingStory):
  NAME = 'flywheel:em:tele.ru'
  URL = 'http://www.tele.ru/stars/private-life/stesha-malikova-filipp-gazmanov-anna-zavorotnyuk-i-drugie-zvezdnye-deti-kotorye-sami-zarabatyvayut-n/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel234Story(_LoadingStory):
  NAME = 'flywheel:em:dota2.vpgame.com-2'
  URL = 'http://dota2.vpgame.com/match/10160929.html?category=dota'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel235Story(_LoadingStory):
  NAME = 'flywheel:em:wowkeren.com'
  URL = 'http://wowkeren.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel236Story(_LoadingStory):
  NAME = 'flywheel:em:starsports.com'
  URL = 'http://starsports.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel237Story(_LoadingStory):
  NAME = 'flywheel:em:consultaholerite.com.br'
  URL = 'http://www.consultaholerite.com.br/consulta_holerite/index.php'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel238Story(_LoadingStory):
  NAME = 'flywheel:em:viva.co.id'
  URL = 'http://viva.co.id'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel239Story(_LoadingStory):
  NAME = 'flywheel:em:uol.com.br'
  URL = 'http://uol.com.br'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel240Story(_LoadingStory):
  NAME = 'flywheel:em:nobubank.com'
  URL = 'http://www.nobubank.com/people'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel241Story(_LoadingStory):
  NAME = 'flywheel:em:es.ccm.net'
  URL = 'http://es.ccm.net/s/como+saber+mi+correo+electronico?qlc#k=88e1521111094b3853b306592f32efc9'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel242Story(_LoadingStory):
  NAME = 'flywheel:em:dapurmasak.com'
  URL = 'http://dapurmasak.com'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel243Story(_LoadingStory):
  NAME = 'flywheel:em:beanactuary.org'
  URL = 'http://www.beanactuary.org/why/'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel244Story(_LoadingStory):
  NAME = 'flywheel:em:sharelagu.info'
  URL = 'http://sharelagu.info'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel245Story(_LoadingStory):
  NAME = 'flywheel:em:intoday.in'
  URL = 'http://intoday.in'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY

class LoadFlywheel246Story(_LoadingStory):
  NAME = 'flywheel:em:koolwap.in'
  URL = 'http://koolwap.in'
  SUPPORTED_PLATFORMS = platforms.MOBILE_ONLY
