/** @type {String} */
var MSG_WHAT = ('What:');
/** @type {String} */
var MSG_WHERE = ('Where:');
/** @type {String} */
var MSG_WHO = ('Who:');
/** @type {String} */
var MSG_GOING = ('Going?');
/** @type {String} */
var MSG_JS_OBSOLETE = ('Google Calendar is out of date, reloading from the server now.');
/** @type {String} */
var MSG_WRITE_BACK_SUCCESS = ('Changes saved');
/** @type {String} */
var MSG_WRITE_BACK_FAILURE = ('Error saving changes, please try again');
/** @type {String} */
var MSG_SERVER_ERROR_TITLE = ('Sorry');
/** @type {String} */
var MSG_FRONTEND_ERROR_UNSPECIFIED_TITLE = ('Sorry, Calendar is unavailable right now');
/** @type {String} */
var MSG_FRONTEND_ERROR_UNSPECIFIED_BODY = ('Oops, we\047re sorry, Google Calendar is temporarily unavailable. You can cross your fingers and try again in a few minutes, or take a look at our \074a href\075\042http://www.google.com/support/calendar/bin/topic.py?topic\0758580\042\076help center\074/a\076 for more information.');
/** @type {String} */
var MSG_FRONTEND_ERROR_RENDER_TITLE = ('Sorry, we cannot load your calendar right now');
/** @type {String} */
var MSG_FRONTEND_ERROR_RENDER_BODY = MSG_FRONTEND_ERROR_UNSPECIFIED_BODY;
/** @type {String} */
var MSG_FRONTEND_ERROR_EVENT_TITLE = ('Sorry, we cannot load this event');
/** @type {String} */
var MSG_FRONTEND_ERROR_EVENT_BODY = ('Oops, we\047re having trouble loading this event. The event might have been deleted, or you might not have access to it any more. Cross your fingers and try again in a few minutes, or talk to the person who set up this event.');
/** @type {String} */
var MSG_FRONTEND_ERROR_EMBED_SPURIOUS_TITLE = ('Sorry, we cannot load this calendar');
/** @type {String} */
var MSG_FRONTEND_ERROR_EMBED_SPURIOUS_BODY = MSG_FRONTEND_ERROR_UNSPECIFIED_BODY;
/** @type {String} */
var MSG_FRONTEND_ERROR_EMBED_ACCESS_TITLE = MSG_FRONTEND_ERROR_EMBED_SPURIOUS_TITLE;
/** @type {String} */
var MSG_FRONTEND_ERROR_EMBED_ACCESS_BODY = ('Sorry, this calendar does not have public access enabled. If you are the calendar owner, you can make this calendar public on the calendar sharing settings page.');
/** @type {String} */
var MSG_EMBEDDABLE_PUBLIC_WARNING = ('WARNING: Public access to this calendar has not been enabled. You should change your sharing settings before sharing this address with others.');
/** @type {String} */
var MSG_UNSPECIFIED_ERROR = ('Sorry, we cannot communicate with the server right now');
/** @type {String} */
var MSG_INVITATION_BAD_EMAILS = ('The following email addresses are invalid:');
/** @type {String} */
var MSG_INVITATION_FAILED_EMAILS = ('We were unable to send to the following email addresses:');
/** @type {String} */
var MSG_INVITATION_SUCCEEDED_EMAILS = ('We successfully sent to the following email addresses:');
/** @type {String} */
var MSG_INVITATION_ERROR_TITLE = ('Invitation Status');
/** @return {String} */
function MSG_IMPORT_FAILURE_BAD_URL(ph0) {
  return ('Sorry, ' + ph0 + ' is not a valid URL.');
}
/** @type {String} */
var MSG_IMPORT_CALENDAR_IN_PROGRESS = ('Adding calendar...');
/** @type {String} */
var MSG_IMPORT_ERROR_TITLE = ('Import failed');
/** @return {String} */
function MSG_FAILED_IMPORT_URL(ph0) {
  return ('Failed to import calendar from \042' + ph0 + '\042.');
}
/** @type {String} */
var MSG_ARE_YOU_COMING = ('Are you coming?');
/** @type {String} */
var MSG_ARE_YOU_COMING_PLEASE_CHOOSE_ONE = ('Please choose \047Yes\047 or \047No\047 or \047Maybe\047.');
/** @type {String} */
var MSG_MORE_DETAILS = ('more details');
/** @type {String} */
var MSG_EDIT_EVENT_DETAILS = ('edit event details');
/** @return {String} */
function MSG_CALENDAR_ACCESS_TITLE(ph0) {
  return ('Give access to ' + ph0);
}
/** @return {String} */
function MSG_CALENDAR_ACCESS_BODY(ph0) {
  return ('Press \042Save\042 to give ' + ph0 + ' access to your calendar.');
}
/** @return {String} */
function MSG_CALENDAR_ACCESS_GIVE(ph0) {
  return ('To give ' + ph0 + ' access to your calendar, choose the access level below and click save.');
}
/** @return {String} */
function MSG_CALENDAR_ACCESS_ERROR(ph0) {
  return ('Sorry, this is not a Calendar user: ' + ph0);
}
/** @type {function} */
var MSG_CALENDAR_ADD_ERROR = MSG_CALENDAR_ACCESS_ERROR;
/** @type {String} */
var MSG_CALENDAR_ADD_TITLE = ('Do you want to add this calendar?');
/** @return {String} */
function MSG_CALENDAR_ADD_BODY(ph0) {
  return ('Would you like to add calendar: ' + ph0 + '?');
}
/** @return {String} */
function MSG_CALENDAR_TRANSFER_ADD_BODY(ph0, ph1) {
  return ('Would you like to transfer access from ' + ph0 + ' and add calendar: ' + ph1 + '?');
}
/** @type {String} */
var MSG_CALENDAR_ADD_YES = ('Yes, add this calendar');
/** @type {String} */
var MSG_CALENDAR_ADD_NO = ('No, do not add this calendar');
/** @type {String} */
var MSG_STILL_SAVING = ('(Still saving changes...)');
/** @type {String} */
var MSG_VIEW_ON_CAL = ('view on my calendar');
/** @type {String} */
var MSG_COPY_TO_MY_CAL = ('copy to my calendar');
/** @type {String} */
var MSG_ORGANIZER = ('Creator:');
/** @return {String} */
function MSG_CREATOR_ORGANIZER_SAME(ph0) {
  return (ph0);
}
/** @return {String} */
function MSG_CREATOR_ORGANIZER_PAIR(ph0, ph1) {
  return (ph0 + ' for ' + ph1);
}
/** @type {String} */
var MSG_WHICH_CALENDAR = ('Which Calendar:');
/** @type {String} */
var MSG_NO_DATES = ('Please specify a start and end date');
/** @type {String} */
var MSG_BAD_START_END = ('Sorry, you can\047t create an event that ends before it starts.');
/** @type {String} */
var MSG_EXPAND_ALL = ('Expand All');
/** @type {String} */
var MSG_COLLAPSE_ALL = ('Collapse All');
/** @return {String} */
function MSG_CULLED_EVENTS(ph0) {
  return ('+' + ph0 + ' more');
}
/** @type {String} */
var MSG_ACCEPT = ('Yes');
/** @type {String} */
var MSG_DECLINE = ('No');
/** @type {String} */
var MSG_TENTATIVE = ('Maybe');
/** @type {String} */
var MSG_DELETE = ('Delete');
/** @type {String} */
var MSG_YES = ('\046nbsp;\046nbsp;\046nbsp;Yes\046nbsp;\046nbsp;\046nbsp;');
/** @type {String} */
var MSG_NO = ('\046nbsp;\046nbsp;\046nbsp;No\046nbsp;\046nbsp;\046nbsp;');
/** @type {String} */
var MSG_MAP = ('map');
/** @type {String} */
var MSG_SAVE = ('Save');
/** @type {String} */
var MSG_SAVE_OVERLAPPING_INSTANCES = ('Yes, create overlapping events');
/** @type {String} */
var MSG_SAVE_ONE_INSTANCE = ('No, just one');
/** @type {String} */
var MSG_CANCEL = ('Cancel');
/** @type {String} */
var MSG_OK = ('\046nbsp;\046nbsp;\046nbsp;OK\046nbsp;\046nbsp;\046nbsp;');
/** @type {String} */
var MSG_ERROR = ('Error');
/** @type {String} */
var MSG_DELETE_EVENT_CONFIRMATION_TITLE = ('Delete Event?');
/** @return {String} */
function MSG_ARE_YOU_SURE_YOU_WANT_TO_DELETE_AND_MAYBE_NOTIFY(ph0) {
  return ('Do you want to notify your guests that you\047re cancelling \042' + ph0 + '\042?');
}
/** @return {String} */
function MSG_ARE_YOU_SURE_YOU_WANT_TO_DELETE(ph0) {
  return ('Are you sure you want to delete \042' + ph0 + '\042?');
}
/** @type {String} */
var MSG_DELETE_EVENT_AND_NOTIFY_BUTTON = ('Delete \046amp; notify guests');
/** @type {String} */
var MSG_DELETE_EVENT_BUTTON_NO_NOTIFY = ('Delete without notifying guests');
/** @type {String} */
var MSG_DELETE_EVENT_BUTTON_YES = MSG_DELETE;
/** @type {String} */
var MSG_DELETE_EVENT_BUTTON_NO = MSG_CANCEL;
/** @type {String} */
var MSG_DELETE_EVENT_SUCCESS = ('Event deleted');
/** @type {String} */
var MSG_MY_CALENDARS = ('My Calendars');
/** @type {String} */
var MSG_FAVORITE_CALENDARS = ('Other Calendars');
/** @type {String} */
var MSG_CALENDARS_DEFAULT_QUERY = ('Search public calendars');
/** @type {String} */
var MSG_CREATE_EVENT = ('Create Event');
/** @type {String} */
var MSG_QUICK_ADD = ('Quick Add');
/** @type {String} */
var MSG_QUICK_ADD_EXAMPLE = ('e.g., Dinner with Michael 7pm tomorrow');
/** @type {String} */
var MSG_QUICK_ADD_FULL_EXAMPLE = ('e.g., 7pm Dinner at Pancho\047s');
/** @type {String} */
var MSG_QUICK_ADD_HOLIDAY_EXAMPLE = ('e.g., Mom\047s birthday');
/** @type {String} */
var MSG_QUICK_ADD_ERRAND_EXAMPLE = ('e.g., Breakfast at Tiffany\047s');
/** @type {String} */
var MSG_QUICK_ADD_TRIP_EXAMPLE = ('e.g., Visiting in NYC');
/** @type {String} */
var MSG_QUICK_ADD_BUTTON_ALT_TEXT = ('Add now');
/** @type {String} */
var MSG_NO_SUBJECT = ('(No Subject)');
/** @return {String} */
function MSG_SHOWING_EVENTS_AFTER(ph0) {
  return ('Showing events after ' + ph0);
}
/** @return {String} */
function MSG_SHOWING_EVENTS_UNTIL(ph0) {
  return ('Showing events until ' + ph0);
}
/** @type {String} */
var MSG_LOOK_FOR_EARLIER_EVENTS = ('Look for earlier events');
/** @type {String} */
var MSG_LOOK_FOR_LATER_EVENTS = ('Look for more');
/** @type {String} */
var MSG_BACK_TO_CALENDAR = ('\046laquo; Back to Calendar');
/** @type {String} */
var MSG_DISCARD_CHANGES = ('Your event has not been saved.');
/** @type {String} */
var MSG_DISCARD_DETAILS_CHANGES = ('Your changes have not been saved.');
/** @type {String} */
var MSG_TODAY_BUTTON_LABEL = ('Today');
/** @type {String} */
var MSG_TAB_SEARCH = ('Search Results');
/** @type {String} */
var MSG_TAB_DAY = ('Day');
/** @type {String} */
var MSG_TAB_CUSTOM = ('Custom View');
/** @type {String} */
var MSG_TAB_WEEK = ('Week');
/** @type {String} */
var MSG_TAB_MONTH = ('Month');
/** @type {String} */
var MSG_TAB_LIST = ('Agenda');
/** @type {String} */
var MSG_PAST_EVENT = ('This event is already over, are you sure you want to create it?');
/** @type {String} */
var MSG_NO_EMPTY_URL = ('Please enter a Public Calendar Address');
/** @type {String} */
var MSG_PROBLEMS_WITH_EVENT_FORM = ('There are some problems with this event.');
/** @return {String} */
function MSG_BAD_ATTENDEES(ph0) {
  return ('The following guest email addresses are invalid: \074blockquote\076\074strong\076 ' + ph0 + ' \074/strong\076\074/blockquote\076 \074p\076');
}
/** @type {String} */
var MSG_BAD_ATTENDEES_NOHTML = ('The following are not valid email addresses:');
/** @return {String} */
function MSG_CREATE_EVENT_DATE(ph0, ph1) {
  return ('Added ' + ph0 + ' on ' + ph1 + '.');
}
/** @return {String} */
function MSG_CREATE_EVENT_DATETIME(ph0, ph1, ph2) {
  return ('Added ' + ph0 + ' on ' + ph1 + ' at ' + ph2 + '.');
}
/** @return {String} */
function MSG_OVERLAPPING_OCCURRENCES(ph0, ph1) {
  return ('This event is ' + ph0 + ' ' + ph1 + ' long but repeats more often than that.');
}
/** @return {String} */
function MSG_OVERLAPPING_OCCURRENCES_QUESTION(ph0) {
  return ('Are you sure you want multiple ' + ph0 + ' day long events?');
}
/** @type {String} */
var MSG_BACK_TO_EDIT = ('\046laquo; Edit');
/** @type {String} */
var MSG_SETTINGS_TITLE = ('Calendar Settings');
/** @type {String} */
var MSG_GENERAL = ('General');
/** @type {String} */
var MSG_CALENDARS = ('Calendars');
/** @type {String} */
var MSG_NOTIFICATION = ('Notifications');
/** @type {String} */
var MSG_SETTING_FAVORITE_CALENDARS = MSG_FAVORITE_CALENDARS;
/** @type {String} */
var MSG_SETTING_WRITING = ('Saving settings...');
/** @type {String} */
var MSG_SETTING_RELOADING = ('Reloading Settings... Please wait');
/** @return {String} */
function MSG_SETTING_ERROR_NO_ACCOUNT(ph0) {
  return (ph0 + ' does not have a Google Calendar account');
}
/** @return {String} */
function MSG_SETTING_ERROR_NO_ACCESS(ph0) {
  return ('You do not have access to ' + ph0 + '\047s calendar');
}
/** @type {String} */
var MSG_SETTING_ERROR_BAD_KEY = ('The one time calendar access transfer key is invalid');
/** @type {String} */
var MSG_SETTING_ERROR_ADD_YOURSELF = ('You cannot add your own calendar. If you want to see events on your calendar, ensure that the checkbox next to it is checked under \042My Calendars\042.');
/** @type {String} */
var MSG_SETTING_ERROR_REMOVE_YOURSELF = ('You cannot remove your own calendar. If you do not want to see events on your calendar, you can un-check the checkbox next to it under \042My Calendars\042.');
/** @return {String} */
function MSG_SETTING_ERROR_ADD_CALENDAR_TITLE(ph0) {
  return ('Cannot Add Calendar \042' + ph0 + '\042');
}
/** @return {String} */
function MSG_SETTING_ERROR_REMOVE_CALENDAR_TITLE(ph0) {
  return ('Cannot Remove Calendar \042' + ph0 + '\042');
}
/** @type {String} */
var MSG_SETTINGS_CALENDARS_NAME = ('Calendar');
/** @return {String} */
function MSG_SETTINGS_CALENDARS_CALENDAR(ph0) {
  return ('Calendar \047' + ph0 + '\047');
}
/** @type {String} */
var MSG_SETTINGS_CALENDARS_SHARING = ('Sharing');
/** @type {String} */
var MSG_SETTINGS_CALENDARS_ROW_SHARED = ('Shared: Edit settings');
/** @type {String} */
var MSG_SETTINGS_CALENDARS_ROW_NOT_SHARED = ('Share this calendar');
/** @type {String} */
var MSG_SETTINGS_NOTIFY_EMAIL = ('Email');
/** @type {String} */
var MSG_SETTINGS_NOTIFY_SMS = ('SMS');
/** @type {String} */
var MSG_SETTINGS_NOTIFY_POPUP = ('Pop-up');
/** @type {String} */
var MSG_TIMEZONE = ('Your current time zone:');
/** @type {String} */
var MSG_CALENDAR_TIMEZONE = ('Calendar Time Zone:');
/** @type {String} */
var MSG_CALENDAR_TIMEZONE_DESC = ('Please first select a country to select the right set of timezones. To see all timezones, check the box instead.');
/** @type {String} */
var MSG_SHARE_WITH_DOMAIN = ('Share with my domain:');
/** @type {String} */
var MSG_SHARE_WITH_PERSON = ('Share my calendar with this person');
/** @type {String} */
var MSG_CALENDAR_OWNER = ('Calendar Owner:');
/** @type {String} */
var MSG_DATE_FORMAT = ('Date format:');
/** @type {String} */
var MSG_TIME_FORMAT = ('Time format:');
/** @type {String} */
var MSG_FIRST_WEEKDAY = ('Week starts on:');
/** @type {String} */
var MSG_SHOW_WEEKENDS = ('Show weekends:');
/** @type {String} */
var MSG_SETTINGS_DEFAULT_VIEW = ('Default view:');
/** @type {String} */
var MSG_SETTINGS_CUSTOM_VIEW = ('Custom view:');
/** @type {String} */
var MSG_SETTING_CUSTOM_VIEW_2_DAYS = ('Next 2 Days');
/** @type {String} */
var MSG_SETTING_CUSTOM_VIEW_3_DAYS = ('Next 3 Days');
/** @type {String} */
var MSG_SETTING_CUSTOM_VIEW_4_DAYS = ('Next 4 Days');
/** @type {String} */
var MSG_SETTING_CUSTOM_VIEW_5_DAYS = ('Next 5 Days');
/** @type {String} */
var MSG_SETTING_CUSTOM_VIEW_6_DAYS = ('Next 6 Days');
/** @type {String} */
var MSG_SETTING_CUSTOM_VIEW_7_DAYS = ('Next 7 Days');
/** @type {String} */
var MSG_SETTING_CUSTOM_VIEW_2_WEEKS = ('Next 2 Weeks');
/** @type {String} */
var MSG_SETTING_CUSTOM_VIEW_3_WEEKS = ('Next 3 Weeks');
/** @type {String} */
var MSG_SETTING_CUSTOM_VIEW_4_WEEKS = ('Next 4 Weeks');
/** @type {String} */
var MSG_SETTINGS_SHOW_DECLINED_EVENTS = ('Show events you have declined:');
/** @type {String} */
var MSG_SETTINGS_AUTOMATICALLY_ADD_INVITATIONS = ('Automatically add invitations to my calendar:');
/** @type {String} */
var MSG_SETTINGS_NO_INVITATIONS = ('No, only show invitations to which I have responded');
/** @type {String} */
var MSG_SETTINGS_GAIA = ('Google Account settings:');
/** @return {String} */
function MSG_SETTINGS_GAIA_LINK(ph0, ph1) {
  return ('Visit your ' + ph0 + 'Google Account settings' + ph1 + ' to reset your password, change your security question, or learn about access to other Google services');
}
/** @type {String} */
var MSG_SETTINGS_DISPLAY_LANGUAGE = ('Language:');
/** @type {String} */
var MSG_SETTINGS_NO_LANGUAGE_CHOSEN = ('Choose a language...');
/** @type {String} */
var MSG_SETTINGS_GAIA_HOSTED = ('Change password:');
/** @return {String} */
function MSG_SETTINGS_GAIA_LINK_HOSTED(ph0, ph1) {
  return ('Follow this link ' + ph0 + 'Change Password' + ph1 + ' to reset your password.');
}
/** @type {String} */
var MSG_DELETE_CALENDAR_OPTION = MSG_DELETE;
/** @type {String} */
var MSG_REMOVE_CALENDAR_OPTION = ('Remove');
/** @type {String} */
var MSG_REMOVE_OR_DELETE_CALENDAR_CONFIRMATION_TITLE = ('Remove or Delete Calendar');
/** @return {String} */
function MSG_REMOVE_OR_DELETE_CALENDAR_CONFIRMATION_TEXT(ph0, ph1) {
  return ('You can choose to remove \074b\076' + ph0 + '\074/b\076 from your list of calendars,\074BR\076 or permanently delete the \074b\076' + ph1 + '\074/b\076 calendar.');
}
/** @type {String} */
var MSG_DELETE_CALENDAR_CONFIRMATION_TITLE = ('Delete Calendar');
/** @type {String} */
var MSG_DELETE_CALENDAR_CONFIRMATION_TEXT = ('Are you sure you want to permanently delete all events on this calendar?');
/** @type {String} */
var MSG_REMOVE_CALENDAR_CONFIRMATION_TITLE = ('Remove Calendar');
/** @return {String} */
function MSG_REMOVE_CALENDAR_CONFIRMATION_TEXT(ph0) {
  return ('Are you sure you want to remove \074b\076' + ph0 + '\074/b\076 from your list of calendars?');
}
/** @type {String} */
var MSG_REMOVE_FAILED_TITLE = ('Remove failed');
/** @return {String} */
function MSG_REMOVE_FAILED_TEXT(ph0) {
  return ('You were not able to remove \074B\076' + ph0 + '\074/B\076 from your list of calendars because you are the only person left with full access to the calendar.\074BR\076 You may either grant full access to another user to carry on the legacy of this calendar, or you may delete it altogether.');
}
/** @type {String} */
var MSG_UNHIDE_CALENDAR = ('Show');
/** @type {String} */
var MSG_HIDE_CALENDAR = ('Hide');
/** @type {String} */
var MSG_ADD_NEW_CALENDAR = ('Create new calendar');
/** @type {String} */
var MSG_ADD_FAVORITE_CALENDAR = ('Add calendar');
/** @type {String} */
var MSG_PREVIEW = ('Preview');
/** @type {String} */
var MSG_ADD_CONTACT = ('Enter the email address of another person to view their calendar. Not all of your contacts will have calendar information that is shared with you, but you can invite them to create a Google Calendar account, or share their calendar with you.');
/** @type {String} */
var MSG_ADD_CONTACT_NAME = ('Contact Email');
/** @type {String} */
var MSG_ADD_CONTACT_BUTTON = ('Add');
/** @type {String} */
var MSG_ADD_CALENDAR_BUTTON = MSG_ADD_CONTACT_BUTTON;
/** @type {String} */
var MSG_SEARCH_BUTTON = ('Search');
/** @type {String} */
var MSG_INVITE_USER_TITLE = ('Invite Person');
/** @type {String} */
var MSG_REQUEST_ACCESS_TITLE = ('Request Access');
/** @type {String} */
var MSG_CAL_INVITE = ('Type in a brief message to invite this person to Google Calendar.');
/** @type {String} */
var MSG_CAL_REQUEST = ('Type in a brief message to request access to this calendar.');
/** @type {String} */
var MSG_CAL_INVITE_NOTE = ('Add a note to the invitation (optional):');
/** @type {String} */
var MSG_CAL_REQUEST_NOTE = ('Add a note to the request (optional):');
/** @type {String} */
var MSG_CAL_INVITE_NOTE_TEMPLATE = ('I\047ve been using Google Calendar to organize my calendar, find interesting events, and share my schedule with friends and family members. I thought you might like to use Google Calendar, too.');
/** @type {String} */
var MSG_CAL_REQUEST_NOTE_TEMPLATE = ('I\047ve been using Google Calendar to organize my schedule, find interesting events, and share my schedule with friends and family members. I\047d like to be able to view your calendar to make scheduling things together even easier.');
/** @type {String} */
var MSG_SEND_CAL_INVITE = ('Send Invite');
/** @type {String} */
var MSG_SEND_CAL_REQUEST = ('Send Request');
/** @type {String} */
var MSG_MESSAGE_SENT = ('Message sent');
/** @type {String} */
var MSG_ADD_EXTERNAL = ('If you know the address to a calendar (in iCal format), you can type in the address here.');
/** @type {String} */
var MSG_ADD_CALENDAR_SEARCH_INDEX = ('Allow others to find this public calendar via Google Calendar search?');
/** @type {String} */
var MSG_REMIND_ME = ('Reminder');
/** @type {String} */
var MSG_DEFAULT_REMINDER = ('Event reminders:');
/** @type {String} */
var MSG_DEFAULT_REMINDER_CAPTION = ('Unless otherwise specified by the individual event.');
/** @return {String} */
function MSG_BY_DEFAULT_REMIND_ME_BEFORE_EACH_EVENT(ph0) {
  return ('By default, remind me ' + ph0 + ' before each event');
}
/** @type {String} */
var MSG_MENU_CREATE_EVENT = ('Create event on this calendar');
/** @type {String} */
var MSG_MENU_SHOW_ONLY = ('Display only this Calendar');
/** @type {String} */
var MSG_MENU_HIDE_THIS_CALENDAR = ('Hide this calendar from the list');
/** @type {String} */
var MSG_MENU_SETTINGS = ('Calendar settings');
/** @type {String} */
var MSG_ADD_CALENDAR_FAV_TITLE = ('Add Other Calendar');
/** @type {String} */
var MSG_ADD_CALENDAR = ('Add Calendar');
/** @type {String} */
var MSG_DETAILS_TAB_DESCRIPTION = ('Calendar Details');
/** @type {String} */
var MSG_DETAILS_TAB_SHARING = MSG_SETTINGS_CALENDARS_ROW_NOT_SHARED;
/** @return {String} */
function MSG_DETAILS_TITLE(ph0) {
  return (ph0 + ' Details');
}
/** @type {String} */
var MSG_DETAILS_TITLE_NEW = ('Create New Calendar');
/** @type {String} */
var MSG_DETAILS_LOADING = ('Loading...');
/** @type {String} */
var MSG_DETAILS_CREATE = ('Create Calendar');
/** @type {String} */
var MSG_DETAILS_NAME = ('Calendar Name:');
/** @type {String} */
var MSG_DETAILS_DESCRIPTION = ('Description:');
/** @type {String} */
var MSG_DETAILS_LOCATION = ('Location:');
/** @type {String} */
var MSG_DETAILS_LOCATION_HINT = ('e.g. \042San Francisco\042 or \042New York\042 or \042USA.\042 Specifying a general location will help people find events on your calendar (if it\047s public)');
/** @type {String} */
var MSG_DETAILS_SHARING_USER = ('Person');
/** @type {String} */
var MSG_DETAILS_SHARING_ADD_EDIT_USER = ('Share with other people or edit who has access.');
/** @type {String} */
var MSG_DETAILS_SHARING_ADD_NEW_USER = ('Add a NEW Person:');
/** @type {String} */
var MSG_DETAILS_SHARING_PERMISSION = ('Has Permission To');
/** @type {String} */
var MSG_DETAILS_SHARING_DELETE = MSG_DELETE;
/** @type {String} */
var MSG_DETAILS_SHARING_EVERYBODY = ('Share with everyone:');
/** @type {String} */
var MSG_DETAILS_SHARING_EVERYBODY_CONFIRM_TITLE = ('Warning');
/** @type {String} */
var MSG_DETAILS_SHARING_EVERYBODY_CONFIRM_BODY = ('Making your calendar public will make all events visible to the world, including via Google search. Are you sure?');
/** @type {String} */
var MSG_DETAILS_SHARING_EVERYBODY_CONFIRM_YES = MSG_ACCEPT;
/** @type {String} */
var MSG_DETAILS_SHARING_EVERYBODY_CONFIRM_NO = MSG_DECLINE;
/** @type {String} */
var MSG_DETAILS_SHARING_SPECIFIC = ('Share with specific people:');
/** @type {String} */
var MSG_DETAILS_AUTO_ACCEPT_HEADER = ('Auto-accept invitations');
/** @type {String} */
var MSG_DETAILS_AUTO_ACCEPT_DESC = ('Calendars for resources like conference rooms can automatically accept invitations from people with whom the calendar is shared when there are no conflicting events.');
/** @type {String} */
var MSG_DETAILS_AUTO_ACCEPT_YES = ('Yes, auto-accept invitations that do not conflict.');
/** @type {String} */
var MSG_DETAILS_AUTO_ACCEPT_NO = ('No, do not automatically accept invitations.');
/** @type {String} */
var MSG_DETAILS_POPUP_FEED_TITLE = ('Calendar Address');
/** @type {String} */
var MSG_DETAILS_POPUP_FEED_ICAL = ('Please use the following address to access your calendar from other applications. You can copy and paste this into any calendar product that supports the iCal format.');
/** @type {String} */
var MSG_DETAILS_POPUP_FEED_XML = ('Please use the following address to access your calendar from other applications. You can copy and paste this into any feed reader.');
/** @return {String} */
function MSG_DETAILS_POPUP_FEED_HTML(ph0, ph1) {
  return ('Please use the following address to access your calendar in any web browser. \074br\076\074br\076 ' + ph0 + ' \074br\076\074br\076 You can embed Google Calendar in your website or blog. Use our \074a target\075\042_blank\042 href\075\042' + ph1 + '\042\076configuration tool\074/a\076 to generate the HTML you need.');
}
/** @type {String} */
var MSG_DETAILS_PUBLIC_FEED_DESC = ('This is the address for your calendar. No one can use this link unless you have made your calendar public.');
/** @type {String} */
var MSG_DETAILS_PRIVATE_FEED_DESC = ('This is the private address for this calendar. Don\047t share this address with others unless you want them to see all the events on this calendar.');
/** @type {String} */
var MSG_DETAILS_REQUEST_PRIVATE_FEED = ('Requesting new URL from the server');
/** @type {String} */
var MSG_DETAILS_CHANGE_SHARING_SETTINGS = ('Change sharing settings');
/** @type {String} */
var MSG_DETAILS_ERROR_TITLE = ('Settings Error');
/** @type {String} */
var MSG_DETAILS_INVITE_TITLE = ('Invite People');
/** @type {String} */
var MSG_DISCARD_NEW_CALENDAR = ('Are you sure that you want to discard this new calendar?');
/** @type {String} */
var MSG_DETAILS_ADD_USER = ('Add Person');
/** @type {String} */
var MSG_DETAILS_NO_NAME = ('Sorry, you cannot create a calendar without a name');
/** @type {String} */
var MSG_DETAILS_NO_TIMEZONE = ('Sorry, you cannot save a calendar with no time zone');
/** @type {String} */
var MSG_DETAILS_PRIVATE_FEED = ('Private Address:');
/** @type {String} */
var MSG_DETAILS_ERROR = MSG_ERROR;
/** @type {String} */
var MSG_DETAILS_ERROR_PUBLIC_CALENDAR = ('Cannot add permissions for the public user');
/** @type {String} */
var MSG_DETAILS_ERROR_YOURSELF = ('You can\047t share a calendar with yourself');
/** @return {String} */
function MSG_DETAILS_ERROR_INVALID_USER(ph0) {
  return (ph0 + ' is not a valid email address');
}
/** @type {String} */
var MSG_DETAILS_ERROR_SHARE_NOT_CALENDAR = ('The following people do not have Google Calendar accounts. Would you like to invite them to use Google Calendar?');
/** @return {String} */
function MSG_DETAILS_ERROR_PERMISSION(ph0) {
  return ('Sorry, you don\047t have permission to edit calendar: ' + ph0);
}
/** @type {String} */
var MSG_HISTORY_MAIN_CALENDAR = ('Calendar Main View');
/** @type {String} */
var MSG_HISTORY_VIEW_EVENT = ('Calendar View Event');
/** @return {String} */
function MSG_HISTORY_SETTINGS(ph0) {
  return ('Calendar Settings Tab ' + ph0);
}
/** @return {String} */
function MSG_HISTORY_ADD(ph0) {
  return ('Calendar Add Tab ' + ph0);
}
/** @type {String} */
var MSG_ACCESS_LEVEL_OWNER = ('Make changes AND manage sharing');
/** @type {String} */
var MSG_ACCESS_LEVEL_EDITOR = ('Make changes to events');
/** @type {String} */
var MSG_ACCESS_LEVEL_CONTRIBUTOR = ('Contribute');
/** @type {String} */
var MSG_ACCESS_LEVEL_RESPOND = ('Respond');
/** @type {String} */
var MSG_ACCESS_LEVEL_READ = ('See all event details');
/** @type {String} */
var MSG_ACCESS_LEVEL_FREEBUSY = ('See free/busy information (no details)');
/** @type {String} */
var MSG_ACCESS_LEVEL_NONE = ('See nothing');
/** @type {String} */
var MSG_ANYONE_CAN = ('Anyone can:');
/** @type {String} */
var MSG_YOU_CAN = ('You can:');
/** @type {String} */
var MSG_CALENDAR_DETAILS_URL_HEADER = ('URL:');
/** @type {String} */
var MSG_CALENDAR_DETAILS_ORIGINAL_NAME_HEADER = ('Original Name:');
/** @type {String} */
var MSG_DOMAIN = ('Domain:');
/** @return {String} */
function MSG_WARNING_INVITEES_OUTSIDE_DOMAIN(ph0, ph1) {
  return ('The following attendees are from outside your domain (' + ph0 + ') \074br\076Are you sure you would like to invite them? \074br\076\074br\076' + ph1);
}
/** @type {String} */
var MSG_WARNING_ACCESS_OUTSIDE_DOMAIN = ('Are you sure you want to give access to the following email address(es) outside this calendar\047s domain?');
/** @type {String} */
var MSG_WARNING_PRIVATEMAGIC_OUTSIDE_DOMAIN = ('Warning: Anyone who sees this link will be able to view all event details of this calendar.');
/** @type {String} */
var MSG_DETAILS_DOMAIN_LEVEL_NONE = ('Do not share with everyone in my domain');
/** @type {String} */
var MSG_DETAILS_DOMAIN_LEVEL_READ = ('Share all information on this calendar with everyone in my domain');
/** @type {String} */
var MSG_DETAILS_DOMAIN_LEVEL_FREE = ('Share only my free / busy information with my domain');
/** @type {String} */
var MSG_DETAILS_PUBLIC_LEVEL_NONE = ('Do not share with everyone');
/** @type {String} */
var MSG_DETAILS_PUBLIC_LEVEL_READ = ('Share all information on this calendar with everyone');
/** @type {String} */
var MSG_DETAILS_PUBLIC_LEVEL_FREE = ('Share only my free / busy information (hide details)');
/** @type {String} */
var MSG_DETAILS_SHARING_CONFIRMATION_TITLE = ('Are you sure?');
/** @type {String} */
var MSG_BAD_BAD_BAD = MSG_DETAILS_SHARING_CONFIRMATION_TITLE;
/** @type {String} */
var MSG_DETAILS_SHARING_CONFIRMATION_MESSAGE = ('Are you sure you want to share this calendar with everyone?\074BR\076 Public calendars appear in Google Calendar searches.');
/** @type {String} */
var MSG_DETAILS_BUTTON_EXPLAIN = ('Use this button on your site to let others subscribe to this calendar.');
/** @type {String} */
var MSG_DETAILS_BUTTON_GETCODE = ('Get the code');
/** @type {String} */
var MSG_DETAILS_BUTTON_POPUP_TITLE = ('Calendar Subscribe Button');
/** @type {String} */
var MSG_DETAILS_BUTTON_POPUP_MESSAGE = ('Copy and paste the following code into your website to create a button that will allow users to subscribe to your calendar:');
/** @type {String} */
var MSG_CHOOSE_HOW_YOU_WANT_TO_RECEIVE_NOTIFICATIONS = ('Choose how you would like to be notified:');
/** @type {String} */
var MSG_ENTER_YOUR_PHONE_NUMBER_TO_RECEIVE_NOTIFICATIONS = ('Notify me on my cell phone:');
/** @type {String} */
var MSG_ENTER_YOUR_USERNAME_TO_RECEIVE_NOTIFICATIONS = MSG_ENTER_YOUR_PHONE_NUMBER_TO_RECEIVE_NOTIFICATIONS;
/** @type {String} */
var MSG_ENTER_YOUR_PHONE_NUMBER_TO_RECEIVE_NOTIFICATIONS_DESC = ('Start by selecting your country, and then enter your phone number and carrier. Finally enter the verification code sent to your phone.');
/** @type {String} */
var MSG_ENTER_YOUR_USERNAME_TO_RECEIVE_NOTIFICATIONS_DESC = ('Start by selecting your country, and then enter your username and carrier domain. Finally enter the verification code sent to your phone');
/** @type {String} */
var MSG_SMS_CODE_VERIFIED_PHONE_NUMBER = ('Phone number successfully validated.');
/** @type {String} */
var MSG_SMS_CODE_VERIFIED_USERNAME = ('SMS email successfully validated.');
/** @type {String} */
var MSG_SMS_CODE_NOT_VERIFIED = ('Phone notifications disabled.');
/** @type {String} */
var MSG_SMS_CODE_VERIFIED_PHONE_DESC = ('Enter a new phone number and carrier to change where your SMS notifications are sent.');
/** @type {String} */
var MSG_SMS_CODE_VERIFIED_USERNAME_DESC = ('Enter a new username and domain to change where your SMS notifications are sent.');
/** @type {String} */
var MSG_SMS_CODE_NOT_VERIFIED_DESC = ('To enable mobile notifications, complete the information below.');
/** @type {String} */
var MSG_PHONE_NUMBER = ('Phone number:');
/** @type {String} */
var MSG_SMS_USERNAME = ('Username:');
/** @type {String} */
var MSG_SEND_SMS_VERIFICATION_CODE = ('Send Verification Code');
/** @type {String} */
var MSG_INPUT_VERIFICATION_CODE = ('Verification code:\074/br\076\074span class\075\042desc\042\076Please enter the verification code sent to your phone\074/span\076');
/** @type {String} */
var MSG_SUBMIT_SMS_VERIFICATION_CODE = ('Finish setup');
/** @type {String} */
var MSG_GET_MOBILE_ACCESS = ('Get mobile access to your calendar:');
/** @type {String} */
var MSG_SMS_STATUS = ('Status:');
/** @type {String} */
var MSG_CARRIER = ('Carrier:');
/** @type {String} */
var MSG_SMS_EMAIL_DOMAIN = ('Email Domain:');
/** @type {String} */
var MSG_PUBLIC_FEED = ('Calendar Address:');
/** @type {String} */
var MSG_NO_PRIVATE_URL_AVAILABLE = ('Disabled');
/** @type {String} */
var MSG_RESET_SECRET_ID = ('Reset Private URLs');
/** @type {String} */
var MSG_LEARN_MORE = ('Learn more');
/** @type {String} */
var MSG_RESET_AREYOUSURE = ('This will invalidate any existing private feeds. Are you sure you want to reset the private feed URL?');
/** @type {String} */
var MSG_SCROLL = ('show off-screen events');
/** @type {String} */
var MSG_SEARCH_ALL_CALS = ('All Calendars');
/** @type {String} */
var MSG_SEARCH_ALL_MY_CALS = ('All My Calendars');
/** @type {String} */
var MSG_SEARCH_ALL_OTHER_CALS = ('All Other Calendars');
/** @type {String} */
var MSG_SEARCH_INVALID_QUERY = ('Invalid search - Please enter a query.');
/** @type {String} */
var MSG_SEARCH_INVALID_QUERY_NO_CALS = ('Invalid search - Please specify at least one calendar to search.');
/** @return {String} */
function MSG_SEARCH_TOO_MANY_RESULTS(ph0) {
  return ('Sorry, you have too many results (' + ph0 + '). Here are a few of them. Please narrow your search.');
}
/** @type {String} */
var MSG_SEARCH_NO_MATCHES = ('No events matched your search.');
/** @type {String} */
var MSG_SEARCH_MATCHING_EVENTS = ('Matching events on this calendar:');
/** @type {String} */
var MSG_SEARCH_NEXT = ('Next');
/** @type {String} */
var MSG_SEARCH_PREV = ('Prev');
/** @type {String} */
var MSG_SEARCH_PREVIOUS = ('Previous');
/** @type {String} */
var MSG_SEARCH_TODAY = MSG_TODAY_BUTTON_LABEL;
/** @return {String} */
function MSG_SEARCH_WHO(ph0) {
  return ('who: ' + ph0);
}
/** @return {String} */
function MSG_SEARCH_WHERE(ph0) {
  return ('where: ' + ph0);
}
/** @return {String} */
function MSG_SEARCH_WHEN(ph0) {
  return ('when: ' + ph0);
}
/** @type {String} */
var MSG_LOADING_APPLICATION = ('(loading)');
/** @return {String} */
function MSG_SEARCH_RESULTS_FOR(ph0) {
  return ('Results for ' + ph0);
}
/** @return {String} */
function MSG_SEARCH_RESULTS_FOR_CALENDARS(ph0, ph1) {
  return ('Results for ' + ph0 + ' on ' + ph1);
}
/** @type {function} */
var MSG_SEARCH_RESULTS_FOR_CALENDAR = MSG_SEARCH_RESULTS_FOR_CALENDARS;
/** @return {String} */
function MSG_SEARCH_SPECIFY(ph0) {
  return ('Try specifying more ' + ph0);
}
/** @return {String} */
function MSG_SEARCH_PUBLIC(ph0) {
  return ('Try searching in ' + ph0);
}
/** @type {String} */
var MSG_SEARCH_OPTIONS = ('options');
/** @type {String} */
var MSG_PUBLIC_EVENTS = ('public events');
/** @return {String} */
function MSG_SEARCH_EVENTS_FOUND(ph0) {
  return (ph0 + ' event(s) found');
}
/** @type {String} */
var MSG_LOADING = MSG_DETAILS_LOADING;
/** @type {String} */
var MSG_REMOVE_ELEMENT = ('remove');
/** @type {String} */
var MSG_REMOVE_ELEMENT_HOVER = ('remove this person from the guest list');
/** @type {String} */
var MSG_UNDO_REMOVE_ELEMENT = ('undo');
/** @type {String} */
var MSG_UNDO_REMOVE_ELEMENT_HOVER = ('add back to the guest list');
/** @type {String} */
var MSG_EDIT_EVENT_LINK = ('edit event');
/** @type {String} */
var MSG_CALENDAR = ('calendar');
/** @type {String} */
var MSG_RECUR_PERIOD_APERIODIC = ('Does not repeat');
/** @type {String} */
var MSG_RECUR_PERIOD_DAILY = ('Daily');
/** @type {String} */
var MSG_RECUR_PERIOD_WEEKLY = ('Weekly');
/** @type {String} */
var MSG_RECUR_PERIOD_MONTHLY = ('Monthly');
/** @type {String} */
var MSG_RECUR_PERIOD_YEARLY = ('Yearly');
/** @type {String} */
var MSG_RECUR_PERIOD_WEEKDAYS = ('Every weekday (Mon-Fri)');
/** @type {String} */
var MSG_RECUR_PERIOD_MON_WED_FRI = ('Every Mon., Wed., and Fri.');
/** @type {String} */
var MSG_RECUR_PERIOD_TUES_THURS = ('Every Tues., and Thurs.');
/** @type {String} */
var MSG_ADD_CALENDAR_TAB_CONTACT = ('Friends\047 Calendars');
/** @type {String} */
var MSG_ADD_CALENDAR_TAB_CUSTOM = ('Public Calendar Address');
/** @type {String} */
var MSG_ADD_CALENDAR_TAB_DIRECTORY = ('Holiday Calendars');
/** @type {String} */
var MSG_ADD_CALENDAR_FUN = ('Fun Calendars');
/** @type {String} */
var MSG_TAB_UPLOAD = ('Import Calendar');
/** @type {String} */
var MSG_UPLOAD_STEP_1 = ('Step 1: Select File');
/** @type {String} */
var MSG_UPLOAD_STEP_1_DESC = ('Choose the file that contains your events. Google Calendar can import event information in iCal or CSV (MS Outlook) format.');
/** @type {String} */
var MSG_UPLOAD_STEP_2 = ('Step 2: Choose Calendar');
/** @type {String} */
var MSG_UPLOAD_STEP_2_DESC = ('Choose the calendar where these events should be saved.');
/** @return {String} */
function MSG_UPLOAD_STEP_3(ph0) {
  return ('Step ' + ph0 + ': Complete Import');
}
/** @type {String} */
var MSG_CALENDAR_URL = MSG_ADD_CALENDAR_TAB_CUSTOM;
/** @type {String} */
var MSG_SEARCH_PUBLIC_CALENDARS = ('Search Public Calendars');
/** @type {String} */
var MSG_SEARCH_PUBLIC_EVENTS = ('Search Public Events');
/** @type {String} */
var MSG_ESFE_SEARCH_INSTRUCTIONS = ('You can search for public calendar information so you can always keep up to date on the latest schedule of your favorite sports team, band, or club.');
/** @type {String} */
var MSG_ESFE_SEARCH_CRITERIA = ('Search Criteria');
/** @type {String} */
var MSG_ESFE_SEARCH_RESULTS_HEADER = ('Results');
/** @type {String} */
var MSG_SHOW_SEARCH_OPTIONS = ('Show\046nbsp;Search\046nbsp;Options');
/** @type {String} */
var MSG_HIDE_SEARCH_OPTIONS = ('Hide\046nbsp;Search\046nbsp;Options');
/** @type {String} */
var MSG_ADVANCED_SEARCH_TITLE = ('Search\046nbsp;Options');
/** @type {String} */
var MSG_SEARCH_BUTTON_LABEL = ('Search My Calendars');
/** @return {String} */
function MSG_CALENDARS_FOUND_MATCHING(ph0, ph1, ph2, ph3) {
  return (ph0 + '-' + ph1 + ' of ' + ph2 + ' calendars found matching \042' + ph3 + '\042');
}
/** @return {String} */
function MSG_NO_CALENDARS_FOUND_MATCHING(ph0) {
  return ('No calendars found matching \042' + ph0 + '\042.');
}
/** @type {String} */
var MSG_ADVANCE_NOTICE_NONE_LABEL = ('No reminder');
/** @type {String} */
var MSG_ADVANCE_NOTICE_5MIN_LABEL = ('5 minutes');
/** @type {String} */
var MSG_ADVANCE_NOTICE_10MIN_LABEL = ('10 minutes');
/** @type {String} */
var MSG_ADVANCE_NOTICE_15MIN_LABEL = ('15 minutes');
/** @type {String} */
var MSG_ADVANCE_NOTICE_20MIN_LABEL = ('20 minutes');
/** @type {String} */
var MSG_ADVANCE_NOTICE_25MIN_LABEL = ('25 minutes');
/** @type {String} */
var MSG_ADVANCE_NOTICE_30MIN_LABEL = ('30 minutes');
/** @type {String} */
var MSG_ADVANCE_NOTICE_45MIN_LABEL = ('45 minutes');
/** @type {String} */
var MSG_ADVANCE_NOTICE_1HR_LABEL = ('1 hour');
/** @type {String} */
var MSG_ADVANCE_NOTICE_2HR_LABEL = ('2 hours');
/** @type {String} */
var MSG_ADVANCE_NOTICE_3HR_LABEL = ('3 hours');
/** @type {String} */
var MSG_ADVANCE_NOTICE_1DAY_LABEL = ('1 day');
/** @type {String} */
var MSG_ADVANCE_NOTICE_2DAY_LABEL = ('2 days');
/** @type {String} */
var MSG_ADVANCE_NOTICE_7DAY_LABEL = ('1 week');
/** @type {String} */
var MSG_EVENT_DESCRIPTION_LABEL = ('Description');
/** @type {String} */
var MSG_EVENT_TITLE_LABEL = ('What');
/** @type {String} */
var MSG_EVENT_TIME_LABEL = ('When');
/** @type {String} */
var MSG_EVENT_RECURRENCE_RANGE_LABEL = ('Range:');
/** @type {String} */
var MSG_EVENT_FIRST_START_LABEL = ('Starts:');
/** @return {String} */
function MSG_EVENT_REPEATS_NOTE(ph0) {
  return ('\046nbsp;\046nbsp;(repeats ' + ph0 + ')');
}
/** @type {String} */
var MSG_EVENT_LOCATION_LABEL = ('Where');
/** @type {String} */
var MSG_EVENT_ORGANIZER = ('Created By');
/** @type {String} */
var MSG_EVENT_SOURCE_LABEL = MSG_SETTINGS_CALENDARS_NAME;
/** @type {String} */
var MSG_EVENT_TRANSPARENT_LABEL = ('Show me as');
/** @type {String} */
var MSG_FREE_BUSY_AS_FREE = ('Available');
/** @type {String} */
var MSG_FREE_BUSY_AS_BUSY = ('Busy');
/** @type {String} */
var MSG_FREE_BUSY_SUMMARY = ('busy');
/** @type {String} */
var MSG_GUEST_COUNT = ('Guest count:\046nbsp;');
/** @return {String} */
function MSG_COUNT_ACCEPTED(ph0) {
  return (ph0 + ' yes');
}
/** @return {String} */
function MSG_COUNT_DECLINED(ph0) {
  return (ph0 + ' no');
}
/** @return {String} */
function MSG_COUNT_TENTATIVE(ph0) {
  return (ph0 + ' maybe');
}
/** @return {String} */
function MSG_COUNT_NEEDS_ACTION(ph0) {
  return (ph0 + ' haven\047t responded');
}
/** @type {String} */
var MSG_COUNT_NEEDS_ACTION_1 = ('1 hasn\047t responded');
/** @type {String} */
var MSG_ICAL_CLASS_DEFAULT_LABEL = ('Default');
/** @type {String} */
var MSG_ICAL_CLASS_PUBLIC_LABEL = ('Public');
/** @type {String} */
var MSG_ICAL_CLASS_PRIVATE_LABEL = ('Private');
/** @return {String} */
function MSG_MAYBE_INVITEES_LABEL(ph0) {
  return ('Maybe \074strong class\075count\076(' + ph0 + ')\074/strong\076');
}
/** @return {String} */
function MSG_ACCEPTED_INVITEES_LABEL(ph0) {
  return ('Yes \074strong class\075count\076(' + ph0 + ')\074/strong\076');
}
/** @return {String} */
function MSG_DECLINED_INVITEES_LABEL(ph0) {
  return ('No \074strong class\075count\076(' + ph0 + ')\074/strong\076');
}
/** @return {String} */
function MSG_NEEDS_ACTION_INVITEES_LABEL(ph0) {
  return ('Awaiting response \074strong class\075count\076(' + ph0 + ')\074/strong\076');
}
/** @type {String} */
var MSG_RESPONSE_WILL_ATTEND = MSG_ACCEPT;
/** @return {String} */
function MSG_RESPONSE_NUM_GUESTS(ph0) {
  return ('+ ' + ph0 + ' guests');
}
/** @type {String} */
var MSG_RESPONSE_WILL_NOT_ATTEND = MSG_DECLINE;
/** @type {String} */
var MSG_RESPONSE_MIGHT_ATTEND = MSG_TENTATIVE;
/** @type {String} */
var MSG_ERROR_TRY_AGAIN_LATER = ('Server failure. Try again later.');
/** @type {String} */
var MSG_NOTICE_EVENT_UPDATED = ('Your event was updated.');
/** @type {String} */
var MSG_NOTICE_EVENT_CREATED = ('Your event was created.');
/** @type {String} */
var MSG_NO_COMMENTS = ('Sorry, nothing to read here. Try \074a href\075\042http://news.google.com/\042 target\075_new\076Google News\074/a\076 if you\047re bored.');
/** @type {String} */
var MSG_NO_DATE = ('Please specify a date for your event');
/** @type {String} */
var MSG_PARTIAL_DATES = ('Please enter a complete start and end time');
/** @type {String} */
var MSG_INVALID_DATE_TITLE = ('Partial Dates');
/** @type {String} */
var MSG_INVALID_DATE_BODY_START = ('Please specify a starting date');
/** @type {String} */
var MSG_INVALID_DATE_BODY_END = ('Please specify an ending date');
/** @type {String} */
var MSG_INVALID_TIME_BODY_START = ('Please specify a starting time');
/** @type {String} */
var MSG_INVALID_TIME_BODY_END = ('Please specify an ending time');
/** @type {String} */
var MSG_INVALID_END_BEFORE_START = ('Please specify an ending time after the start time');
/** @return {String} */
function MSG_BAD_INVITEE(ph0) {
  return ('Sorry, this isn\047t a valid email address: ' + ph0 + '.');
}
/** @type {String} */
var MSG_GOOGLE_MAP_LABEL = MSG_MAP;
/** @type {String} */
var MSG_GOOGLE_MAP_TITLE = ('click to view in Google maps');
/** @type {String} */
var MSG_EDIT_URL_LABEL = ('edit');
/** @type {String} */
var MSG_EDIT_URL_TITLE = ('click to edit the link');
/** @type {String} */
var MSG_COMPOSE_EVENT_PAGE_TITLE = ('New Event');
/** @type {String} */
var MSG_NO_TITLE_WARNING = ('Your event doesn\047t have a proper title. Would you like to create it anyway or return to editing?');
/** @return {String} */
function MSG_NUM_GUESTS(ph0) {
  return ('+' + ph0 + ' guests');
}
/** @type {String} */
var MSG_ONE_GUEST = ('+1 guest');
/** @type {String} */
var MSG_EVENT_VALIDATION_DIALOG_TITLE = ('Your Event');
/** @type {String} */
var MSG_EVENT_DISCARD_YES = ('Discard changes');
/** @type {String} */
var MSG_EVENT_DISCARD_NO = ('Continue editing');
/** @type {String} */
var MSG_CHECK_RESOURCE_AVAILABILITY = ('Check guest and resource availability');
/** @type {String} */
var MSG_SHOW_DESCRIPTION = ('Click to add a description');
/** @type {String} */
var MSG_SEND_NOTIFICATIONS_TITLE = ('Send Update?');
/** @type {String} */
var MSG_SEND_NOTIFICATIONS = ('Would you like to notify guests of your changes?');
/** @type {String} */
var MSG_SEND_INVITATIONS_TITLE = ('Send Invitations?');
/** @type {String} */
var MSG_SEND_INVITATIONS = ('Would you like to send invitations to guests?');
/** @type {String} */
var MSG_SEND_INVITATIONS_TO_NEW_UPDATES_TO_EXISTING_AND_CANCELLATIONS_TO_REMOVED = ('Would you like to send invitations to new guests, updates to existing guests, and cancellations to removed guests?');
/** @type {String} */
var MSG_SEND_INVITATIONS_TO_NEW_AND_UPDATES_TO_EXISTING = ('Would you like to send invitations to new guests and updates to existing guests?');
/** @type {String} */
var MSG_SEND_UPDATES_TO_EXISTING_AND_CANCELLATIONS_TO_REMOVED = ('Would you like to send updates to existing guests and cancellations to removed guests?');
/** @type {String} */
var MSG_SEND_UPDATES_TO_EXISTING = ('Would you like to send updates to existing guests?');
/** @type {String} */
var MSG_SEND_INVITATIONS_TO_NEW_AND_CANCELLATIONS_TO_REMOVED = ('Would you like to send invitations to new guests and cancellations to removed guests?');
/** @type {String} */
var MSG_SEND_INVITATIONS_TO_NEW = ('Would you like to send invitations to new guests?');
/** @type {String} */
var MSG_SEND_CANCELLATIONS_TO_REMOVED = ('Would you like to send cancellations to removed guests?');
/** @type {String} */
var MSG_DO_SEND_NOTIFICATION = ('Send');
/** @type {String} */
var MSG_DONT_SEND_NOTIFICATION = ('Don\047t send');
/** @type {String} */
var MSG_DO_SEND_INVITE = ('Invite');
/** @type {String} */
var MSG_DONT_SEND_INVITE = ('Don\047t Invite');
/** @type {String} */
var MSG_SHOW_LOCATION = ('Click to add a location');
/** @type {String} */
var MSG_ERROR_LOGGED_OUT = ('Sorry, you have been logged out [probably in another window]. Please log in again.');
/** @type {String} */
var MSG_ERROR_LOAD_PREFS = ('Oops, we couldn\047t load your calendar preferences, please try again in a few minutes');
/** @type {String} */
var MSG_ERROR_SAVE_PREFS = ('Failed to save preferences');
/** @type {String} */
var MSG_ERROR_EVENT_SEARCH = ('Sorry, Event Search is unavailable right now. Please try again later.');
/** @type {String} */
var MSG_ERROR_CREATE_EVENT = ('Oops, we couldn\047t create this event, please try again in a few minutes');
/** @type {String} */
var MSG_ERROR_LOAD_EVENT = ('Oops, we couldn\047t load this event, please try again in a few minutes');
/** @type {String} */
var MSG_ERROR_RESPOND_EVENT = ('Failed to respond to event');
/** @type {String} */
var MSG_ERROR_EDIT_EVENT = ('Oops, we couldn\047t edit this event, please try again in a few minutes');
/** @type {String} */
var MSG_ERROR_DELETE_EVENT = ('Oops, we couldn\047t delete this event, please try again in a few minutes');
/** @type {String} */
var MSG_ERROR_LOAD_SCHEDULER = ('Failed to load scheduler data');
/** @type {String} */
var MSG_ENDS = ('Ends:');
/** @type {String} */
var MSG_ENDS_NEVER = ('Never');
/** @type {String} */
var MSG_ERROR_LOAD_DETAILS = ('Oops, we couldn\047t load details for your calendar, please try again in a few minutes');
/** @type {String} */
var MSG_ERROR_SAVE_DETAILS = ('Oops, we couldn\047t save changes, please try again in a few minutes');
/** @type {String} */
var MSG_ERROR_REMOVE_CALENDAR = ('Failed to remove calendar');
/** @type {String} */
var MSG_ERROR_LOAD_CALENDAR = ('Oops, we couldn\047t load your calendar, please try again in a few minutes');
/** @type {String} */
var MSG_ERROR_REFRESH = ('Failed to refresh with the server');
/** @type {String} */
var MSG_ERROR_LOAD_EVENTS = ('Oops, we couldn\047t load your events, please try again in a few minutes');
/** @type {String} */
var MSG_ERROR_QUICKADD = ('Failed to quick add event');
/** @type {String} */
var MSG_ERROR_SEARCH = ('Failed to search my calendar');
/** @type {String} */
var MSG_EVENT_DRAG_NOTIFICATION_TITLE = ('Send Notification?');
/** @type {String} */
var MSG_EVENT_DRAG_NOTIFICATION_BODY = MSG_SEND_NOTIFICATIONS;
/** @type {String} */
var MSG_EVENT_DRAG_NOTIFICATION_YES = MSG_DO_SEND_NOTIFICATION;
/** @type {String} */
var MSG_EVENT_DRAG_NOTIFICATION_NO = MSG_DONT_SEND_NOTIFICATION;
/** @type {String} */
var MSG_EVENT_DRAG_RECUR_TITLE = ('Edit Recurring Event');
/** @type {String} */
var MSG_EVENT_DRAG_RECUR_BODY = ('Would you like to change only this instance of the event, or all events in this series?');
/** @type {String} */
var MSG_EVENT_DRAG_RECUR_BODY_MAYBE_TAIL = ('Would you like to change only this event, all events in the series, or this and all future events in the series?');
/** @type {String} */
var MSG_EVENT_DRAG_RECUR_ALL = ('All events in the series');
/** @type {String} */
var MSG_EVENT_DRAG_RECUR_TAIL = ('All following');
/** @type {String} */
var MSG_EVENT_DRAG_RECUR_ONE = ('Only this instance');
/** @type {String} */
var MSG_EVENT_DELETE_RECUR_TITLE = ('Delete Recurring Event');
/** @type {String} */
var MSG_EVENT_DELETE_RECUR_BODY = ('Would you like to delete only this instance of the event, or all events in this series?');
/** @type {String} */
var MSG_EVENT_DELETE_RECUR_BODY_MAYBE_TAIL = ('Would you like to delete only this event, all events in the series, or this and all future events in the series?');
/** @type {String} */
var MSG_EVENT_DELETE_RECUR_ALL = MSG_EVENT_DRAG_RECUR_ALL;
/** @type {String} */
var MSG_EVENT_DELETE_RECUR_TAIL = MSG_EVENT_DRAG_RECUR_TAIL;
/** @type {String} */
var MSG_EVENT_DELETE_RECUR_ONE = MSG_EVENT_DRAG_RECUR_ONE;
/** @return {String} */
function MSG_RELATIVE_DURATION_HOURS(ph0) {
  return ('(' + ph0 + ' hours ago)');
}
/** @type {String} */
var MSG_RELATIVE_DURATION_1HOUR = ('(1 hour ago)');
/** @return {String} */
function MSG_RELATIVE_DURATION_MINUTES(ph0) {
  return ('(' + ph0 + ' minutes ago)');
}
/** @type {String} */
var MSG_RELATIVE_DURATION_1MINUTE = ('(1 minute ago)');
/** @type {String} */
var MSG_PREV_DAY = ('Prev day');
/** @type {String} */
var MSG_ALL_DAY = ('All day');
/** @type {String} */
var MSG_PUBLISH_EVENT_TITLE = ('Publish Event Button');
/** @type {String} */
var MSG_PUBLISH_EVENT_DESCRIPTION = ('Put this code on your site so that visitors can easily add this event to their Google Calendar.');
/** @type {String} */
var MSG_PRINT_PREVIEW_TITLE = ('Calendar Print Preview');
/** @type {String} */
var MSG_PRINT_BUTTON_TEXT = ('Print');
/** @type {String} */
var MSG_TRY_PRINTING_TO_PDF_INSTEAD = ('To see a more concise version of your calendar, optimized for printing, click the printer icon to the left (Acrobat Reader req\047d)');
/** @type {String} */
var MSG_PRINT_BUTTON_TOOLTIP = ('Print my calendar (shows preview)');
/** @type {String} */
var MSG_SOME_OVERLAID_CALENDARS_FAILED = ('One or more of your selected calendars could not be loaded at this time. You can try to re-select the hidden calendars in a few moments.');
/** @type {String} */
var MSG_UPLOAD_SUCCESSFUL = ('Upload successful');
/** @type {String} */
var MSG_UPLOAD_FILE_MISSING = ('File missing');
/** @type {String} */
var MSG_UPLOAD_TOO_LARGE = ('File upload too big');
/** @type {String} */
var MSG_UPLOAD_FILE_EMPTY = ('Empty uploaded file');
/** @type {String} */
var MSG_UPLOAD_IO_EXCEPTION = ('Processing error during upload');
/** @type {String} */
var MSG_UPLOAD_MALFORMED = ('Unable to process your iCal/CSV file. The file is not properly formatted.');
/** @type {String} */
var MSG_UPLOAD_QUOTA_EXCEEDED = ('Daily quota exceeded');
/** @type {String} */
var MSG_UPLOAD_ALREADY_IMPORTED = ('Some of the events in this file were not imported because you had imported them to Google Calendar before. Other events in this file have been imported.');
/** @type {String} */
var MSG_UNTITLED_CALENDAR = ('Untitled Calendar');
/** @type {String} */
var MSG_PARAMETER_ERROR = ('Parameter Error');
/** @type {String} */
var MSG_INVALID_DATES = ('Invalid date range specified');
/** @return {String} */
function MSG_INVALID_CALENDAR_ID(ph0) {
  return ('Invalid calendar id: ' + ph0);
}
/** @type {String} */
var MSG_INVALID_WKST = ('Week start parameter (wkst) must be between 1 (Sunday) and 7 (Saturday)');
/** @type {String} */
var MSG_INVALID_DISPLAY_MODE = ('Mode parameter (mode) must be either DAY, WEEK, or MONTH');
/** @type {String} */
var MSG_INVALID_HEIGHT = ('Mode parameter (height) is invalid. It should be an integer such as 200');
/** @type {String} */
var MSG_INVALID_BACKGROUND_COLOR = ('Mode parameter (bgcolor) is invalid. It should be a hex color such as #FF0000');
/** @type {String} */
var MSG_INVALID_DISPLAY_MODE_WEEK = ('Week view is not supported. Please use either DAY or MONTH.');
/** @type {String} */
var MSG_INVALID_CHROME = ('Chrome parameter (chrome) must be either ALL, NAVIGATION, or NONE');
/** @type {String} */
var MSG_NO_CALENDAR = ('No valid calendars found. Please check to make sure the URL contains at least one valid calendar id.');
/** @return {String} */
function MSG_UNABLE_TO_FIND_USER_FOR(ph0) {
  return ('Unable to find user for \047' + ph0 + '\047');
}
/** @return {String} */
function MSG_UNABLE_TO_ADD_TO_FAVORITES_LIST_FOR(ph0) {
  return ('Unable to add to favorites list for \047' + ph0 + '\047');
}
/** @return {String} */
function MSG_FAILED_TO_ADD_IMPORTED_CALENDAR(ph0, ph1) {
  return ('Failed to add imported calendar at ' + ph0 + ' for ' + ph1);
}
/** @return {String} */
function MSG_CALENDAR_WILL_BE_INDEXED(ph0) {
  return ('The calendar at ' + ph0 + ' will be indexed by Google Calendar.');
}
/** @type {String} */
var MSG_IMPORTING_CALENDAR_FROM_URL = ('Importing calendar from url...');
/** @type {String} */
var MSG_CAN_ONLY_ADD_TO_FAVORITES_LIST = ('Can only add to favorites list');
/** @type {String} */
var MSG_ERROR_SAVING_SUBSCRIPTIONS = ('Error saving subscriptions.');
/** @type {String} */
var MSG_SUBSCRIPTIONS_SUCCESSFULLY_UPDATED = ('Subscriptions successfully updated.');
/** @return {String} */
function MSG_SMS_COULDNT_SEND_VERIFICATION(ph0, ph1) {
  return ('Could not send SMS verification code to ' + ph0 + ' via ' + ph1);
}
/** @return {String} */
function MSG_SMS_SENDING_VERIFICATION_CODE(ph0, ph1) {
  return ('Sending verification code to ' + ph0 + ' via ' + ph1 + ', please enter the code in the Verification code field.');
}
/** @return {String} */
function MSG_SMS_TOO_MANY_CODE_REQUESTS(ph0, ph1, ph2, ph3) {
  return ('You have requested too many verification codes be sent to ' + ph0 + ' via ' + ph1 + '. Currently we allow at most ' + ph2 + ' per ' + ph3 + '.');
}
/** @type {String} */
var MSG_SMS_VERIFICATION_CODE_INCORRECT = ('Verification code incorrect, please try again');
/** @type {String} */
var MSG_SMS_RESETTING_VERIFICATION_CODE = ('Maximum number of attempts have failed; resetting the verification code. Press Send Verification Code to send the new code to your phone.');
/** @type {String} */
var MSG_SMS_NO_SMS_AVAILABLE = ('SMS is not available on this server.');
/** @type {String} */
var MSG_SMS_INVALID_CARRIER = ('Invalid carrier.');
/** @type {String} */
var MSG_SMS_INVALID_DOMAIN = ('Invalid domain.');
/** @type {String} */
var MSG_SMS_INVALID_PHONE_NUMBER = ('Invalid phone number.');
/** @type {String} */
var MSG_SMS_INVALID_USERNAME = ('Invalid username.');
/** @return {String} */
function MSG_UNABLE_TO_REMOVE_ACCESS_ON_CALENDAR(ph0) {
  return ('Oops, we couldn\047t remove access on \042' + ph0 + '\042, please try again in a few minutes');
}
/** @return {String} */
function MSG_UNABLE_TO_REMOVE_FROM_LIST_FOR_USER(ph0) {
  return ('Unable to remove from list for \042' + ph0 + '\042');
}
/** @return {String} */
function MSG_UNABLE_TO_DELETE_CALENDAR_FOR_USER(ph0) {
  return ('Unable to delete calendar for \042' + ph0 + '\042');
}
/** @type {String} */
var MSG_UNABLE_TO_CONTACT_CALENDAR_SEARCH = ('Unable to contact the calendar search server.\046nbsp; Please try again later.');
/** @type {String} */
var MSG_IMPORT_STATUS_OK = ('Calendar was imported successfully.');
/** @type {String} */
var MSG_IMPORT_STATUS_COULD_NOT_FETCH = ('Could not fetch the url.');
/** @type {String} */
var MSG_IMPORT_STATUS_BAD_DATA_FORMAT = ('The address that you provided did not contain a calendar in a valid iCal format.');
/** @type {String} */
var MSG_IMPORT_STATUS_NO_UNDERLYING_CALENDAR = ('Could not find the calendar to import the url into.');
/** @type {String} */
var MSG_IMPORT_STATUS_BAD_MERGE = ('Warning: did not successfully merge all events.');
/** @type {String} */
var MSG_IMPORT_STATUS_URL_ROBOTED = ('Could not fetch the url because robots.txt prevents us from crawling the url.');
/** @type {String} */
var MSG_IMPORT_STATUS_OVERSIZE = ('Could not fetch the url because its content is bigger than our size limitation');
/** @type {String} */
var MSG_IMPORT_STATUS_ISSUES = ('We could not parse the calendar at the url requested.');
/** @type {String} */
var MSG_NOTIFICATION_TYPE_EVENT_REMINDERS = MSG_DEFAULT_REMINDER;
/** @type {String} */
var MSG_NOTIFICATION_TYPE_NEW_INVITATIONS = ('New invitations:');
/** @type {String} */
var MSG_NOTIFICATION_TYPE_CHANGED_INVITATIONS = ('Changed invitations:');
/** @type {String} */
var MSG_NOTIFICATION_TYPE_CANCELLED_INVITATIONS = ('Cancelled invitations:');
/** @type {String} */
var MSG_NOTIFICATION_TYPE_INVITATION_REPLIES = ('Invitation replies:');
/** @type {String} */
var MSG_NOTIFICATION_TYPE_DAILY_AGENDA = ('Daily agenda:');
/** @return {String} */
function MSG_NOTIFICATION_TYPE_DETAILS_DAILY_AGENDA(ph0) {
  return ('Sent every day at ' + ph0 + ' in your current time zone');
}
/** @type {String} */
var MSG_CARRIER_NAME_NONE = ('Select carrier...');
/** @type {String} */
var MSG_NO_CARRIER_INFORMATION_REQUIRED = ('No carrier information required.');
/** @type {String} */
var MSG_WHAT_CARRIERS_ARE_SUPPORTED = ('What carriers are supported?');
/** @type {String} */
var MSG_CALENDAR_IMPORT_SUBMIT_BUTTON_LABEL = ('Import');
/** @type {String} */
var MSG_HOLIDAY_CALENDARS_LIST_HEADER = MSG_ADD_CALENDAR_TAB_DIRECTORY;
/** @type {String} */
var MSG_BROWSE_CALENDARS_LIST_HEADER = ('Browse Calendars');
/** @type {String} */
var MSG_FUN_CALENDARS_LIST_HEADER = MSG_ADD_CALENDAR_FUN;
/** @type {String} */
var MSG_MOON_PHASES_CALENDAR_TITLE = ('Phases of the Moon');
/** @type {String} */
var MSG_GOOGLE_HOLIDAY_LOGOS_CALENDAR_TITLE = ('Google Doodles');
/** @type {String} */
var MSG_LOCATION = ('Location');
/** @type {String} */
var MSG_SAMPLE_LOCATION = ('e.g., \074b\076East Brunswick, NJ\074/b\076 or \074b\07608816\074/b\076');
/** @type {String} */
var MSG_SHOW_WEATHER_SETTING = ('Show weather based on my location');
/** @type {String} */
var MSG_WEATHER_SETTING_OFF = ('Do not show weather');
/** @type {String} */
var MSG_WEATHER_SETTING_CELSIUS = ('\046deg;C');
/** @type {String} */
var MSG_WEATHER_SETTING_FAHRENHEIT = ('\046deg;F');
/** @return {String} */
function MSG_WEATHER_FORECAST_FOR_CITY(ph0, ph1) {
  return (ph0 + ' in ' + ph1);
}
/** @type {String} */
var MSG_UNSUPPORTED_BROWSER_NO_DATA_TO_SHOW = ('Sorry, you are trying to use Google Calendar with a browser that isn\047t currently supported. Press OK to view our list of currently supported browsers. Press Cancel to continue loading Google Calendar and hope for the best!');
/** @type {String} */
var MSG_UNSUPPORTED_BROWSER_DATA_TO_SHOW = ('Sorry, you are trying to use Google Calendar with a browser that isn\047t currently supported. Press OK to view a read only version of your calendar. Press Cancel to continue loading Google Calendar and hope for the best!');
/** @return {String} */
function MSG_NOTIFY_EVENT_STARTING_LOC_DATE(ph0, ph1, ph2, ph3) {
  return (ph0 + ' is starting at ' + ph1 + ' in ' + ph2 + ' on ' + ph3 + '.');
}
/** @return {String} */
function MSG_NOTIFY_EVENT_STARTING_LOC(ph0, ph1, ph2) {
  return (ph0 + ' is starting at ' + ph1 + ' in ' + ph2 + '.');
}
/** @return {String} */
function MSG_NOTIFY_EVENT_STARTING_DATE(ph0, ph1, ph2) {
  return (ph0 + ' is starting at ' + ph1 + ' on ' + ph2 + '.');
}
/** @return {String} */
function MSG_NOTIFY_EVENT_STARTING(ph0, ph1) {
  return (ph0 + ' is starting at ' + ph1 + '.');
}
/** @type {String} */
var MSG_DATEPICKER_SELECT_A_DATE = ('Select a date');
/** @type {String} */
var MSG_DATEPICKER_SELECT_A_RANGE_OF_DATES = ('Select a range of dates');
/** @type {String} */
var MSG_DATEPICKER_SELECT_DATES = ('Select dates');
/** @type {String} */
var MSG_DATEPICKER_SELECTION_HEADER = ('Selected:');
/** @return {String} */
function MSG_SMS_IN_INFO(ph0, ph1, ph2, ph3) {
  return ('Google Calendar alerts you of upcoming/new/changed events on your calendar. For info: www.google.com/calendar. To cancel, text ' + ph0 + ' to 48368. Standard msg charges apply. Text the following commands for more info: ' + ph1 + ' (next event), ' + ph2 + ' (today\047s events), ' + ph3 + ' (tomorrow\047s events), or text a new event to add it via quick add.');
}
/** @return {String} */
function MSG_SMS_IN_EVENT_CREATED(ph0) {
  return ('Created event: ' + ph0);
}
/** @type {String} */
var MSG_SMS_IN_UNKNOWN_USER = ('Your phone is not registered. Please verify your phone number at http://www.google.com/calendar.');
/** @type {String} */
var MSG_SMS_IN_COULD_NOT_REPLY = ('Sorry, we could not send a message to your sms.');
/** @type {String} */
var MSG_SMS_IN_MESSAGE_UNCLEAR = ('Sorry, we do not understand that command.');
/** @type {String} */
var MSG_SMS_IN_EVENT_CREATION_FAILED = ('We failed to create an event. Please try again later.');
/** @type {String} */
var MSG_SMS_IN_NO_EVENT_FOUND = ('No events.');
/** @type {String} */
var MSG_SMS_IN_NO_NEXT_EVENT_FOUND = ('No events in the next 24 hours.');
/** @type {String} */
var MSG_SMS_IN_NOTIFICATION_DISABLED = ('Google Calendar alerts have been disabled. You can modify your alert settings at http://www.google.com/calendar');
/** @type {String} */
var MSG_SMS_IN_EVENTS_OMITTED = ('Some events have been omitted.');
/** @type {String} */
var MSG_RANGE_SEPARATOR = ('to');
/** @return {String} */
function MSG_DATE_AND_TIMEZONE(ph0, ph1) {
  return (ph0 + ' (' + ph1 + ')');
}
/** @type {String} */
var MSG_CALENDAR_ERROR = ('Calendar Error');
/** @type {String} */
var MSG_FEED_PROCESSING_ERROR = ('Feed Processing Error');
/** @type {String} */
var MSG_CANT_POST_ICAL_FEEDS = ('Calendar Can\047t POST Ical Feeds');
/** @type {String} */
var MSG_CANT_UPDATE_ICAL_FEEDS = ('Calendar Can\047t UPDATE Ical Feeds');
/** @type {String} */
var MSG_CANT_DELETE_ICAL_FEEDS = ('Calendar Can\047t DELETE Ical Feeds');
/** @return {String} */
function MSG_INVALID_CALENDAR_EMAIL(ph0) {
  return (ph0 + ' is not a valid calendar');
}
/** @type {String} */
var MSG_WAIT_FOR_TIMEZONES_TO_LOAD = ('Loading timezones...');
/** @type {String} */
var MSG_SELECT_TIMEZONE = ('Select timezone...');
/** @type {String} */
var MSG_DISPLAY_ALL_TIMEZONES = ('Display all timezones');
/** @type {String} */
var MSG_COUNTRY = ('Country:');
/** @type {String} */
var MSG_NOW_SELECT_A_TIMEZONE = ('Now select a timezone:');
/** @type {String} */
var MSG_CHOOSE_A_DIFFERENT_COUNTRY_TO_SEE_OTHER_TIMEZONES = ('(choose a different country to see other timezones)');
/** @type {String} */
var MSG_SCHEDULER_RESOURCES_NOT_LOADED_1 = ('Resources not loaded yet.');
/** @type {String} */
var MSG_SCHEDULER_RESOURCES_NOT_LOADED_2 = ('Please close and reopen.');
/** @return {String} */
function MSG_SCHEDULER_RESOURCES_NO_MATCH(ph0) {
  return ('No matches for \042' + ph0 + '\042');
}
/** @return {String} */
function MSG_SCHEDULER_RESOURCES_MAX_1(ph0) {
  return ('Showing first ' + ph0 + ' results');
}
/** @type {String} */
var MSG_SCHEDULER_RESOURCES_MAX_2 = ('Please refine or remove your filter');
/** @type {String} */
var MSG_SCHEDULER_ADD_INVITEE = ('Add a person');
/** @type {String} */
var MSG_SCHEDULER_NO_INFO = ('Calendar doesn\047t exist or isn\047t shared.');
/** @type {String} */
var MSG_SCHEDULER_TITLE = ('Find a Time');
/** @type {String} */
var MSG_SCHEDULER_FILTER_LABEL = MSG_UNHIDE_CALENDAR;
/** @type {String} */
var MSG_SCHEDULER_FILTER_OPTION_ALL = ('All Times');
/** @type {String} */
var MSG_SCHEDULER_FILTER_OPTION_WORKING = ('Working Hours');
/** @type {String} */
var MSG_SCHEDULER_FILTER_OPTION_WEEKENDS = ('Only Weekends');
/** @type {String} */
var MSG_SCHEDULER_FILTER_OPTION_EVENINGS = ('Only Evenings');
/** @type {String} */
var MSG_SCHEDULER_ATTENDEES_LABEL = ('Attendees');
/** @type {String} */
var MSG_SCHEDULER_ATTENDEES_ADD = MSG_ADD_CONTACT_BUTTON;
/** @return {String} */
function MSG_SCHEDULER_ATTENDEES_DESC(ph0, ph1, ph2) {
  return (ph0 + ' - ' + ph1 + ' of ' + ph2);
}
/** @return {String} */
function MSG_COULDNT_PARSE_PATH_TO_ICAL_FILE(ph0) {
  return ('Couldn\047t parse path: ' + ph0);
}
/** @type {String} */
var MSG_CONF_PICKER_OPEN = ('Find an available room');
/** @type {String} */
var MSG_CONF_PICKER_HIDE = ('Hide Room Finder');
/** @type {String} */
var MSG_CONF_PICKER_FILTER = ('Filter room');
/** @type {String} */
var MSG_CONF_PICKER_NOTHING_SELECTED = ('Select a room to add.');
/** @type {String} */
var MSG_CONF_PICKER_ADD = ('Add Room');
/** @type {String} */
var MSG_PROMOTION_NEW = ('New!');
/** @return {String} */
function MSG_COPY_TO_CALENDAR(ph0) {
  return ('Copy to ' + ph0);
}
/** @type {String} */
var MSG_YOUR_CARRIER = ('your carrier');
/** @return {String} */
function MSG_INCOMPLETE_CALENDAR_WARNING(ph0) {
  return ('Warning, these calendar(s) may be incomplete since they aren\047t Google Calendar users: ' + ph0);
}
/** @return {String} */
function MSG_INTERVAL_YEAR_PLURAL(ph0) {
  return (ph0 + ' yrs');
}
/** @return {String} */
function MSG_INTERVAL_MONTH_PLURAL(ph0) {
  return (ph0 + ' months');
}
/** @return {String} */
function MSG_INTERVAL_DAY_PLURAL(ph0) {
  return (ph0 + ' days');
}
/** @return {String} */
function MSG_INTERVAL_HOUR_PLURAL(ph0) {
  return (ph0 + ' hrs');
}
/** @return {String} */
function MSG_INTERVAL_MINUTE_PLURAL(ph0) {
  return (ph0 + ' mins');
}
/** @return {String} */
function MSG_INTERVAL_SECOND_PLURAL(ph0) {
  return (ph0 + ' secs');
}
/** @type {String} */
var MSG_INTERVAL_YEAR_SINGULAR = ('1 yr');
/** @type {String} */
var MSG_INTERVAL_MONTH_SINGULAR = ('1 month');
/** @type {String} */
var MSG_INTERVAL_DAY_SINGULAR = ('1 day');
/** @type {String} */
var MSG_INTERVAL_HOUR_SINGULAR = ('1 hr');
/** @type {String} */
var MSG_INTERVAL_MINUTE_SINGULAR = ('1 min');
/** @type {String} */
var MSG_INTERVAL_SECOND_SINGULAR = ('1 sec');
/** @type {String} */
var MSG_SUNDAYS = ('Sundays');
/** @type {String} */
var MSG_MONDAYS = ('Mondays');
/** @type {String} */
var MSG_TUESDAYS = ('Tuesdays');
/** @type {String} */
var MSG_WEDNESDAYS = ('Wednesdays');
/** @type {String} */
var MSG_THURSDAYS = ('Thursdays');
/** @type {String} */
var MSG_FRIDAYS = ('Fridays');
/** @type {String} */
var MSG_SATURDAYS = ('Saturdays');
/** @type {String} */
var MSG_DAYSET_EVERY_DAY = ('day');
/** @type {String} */
var MSG_DAYSET_EVERY_WEEKDAY = ('weekday');
/** @type {String} */
var MSG_DAYSET_DAYS = ('all days');
/** @type {String} */
var MSG_DAYSET_WEEKDAYS = ('weekdays');
/** @type {String} */
var MSG_DOES_NOT_REPEAT = ('This event does not repeat');
/** @type {String} */
var MSG_REPEATS = ('Repeats:');
/** @type {String} */
var MSG_REPEAT_EVERY_DAY = ('Repeat every:');
/** @type {String} */
var MSG_REPEAT_EVERY_PLURAL_DAY = MSG_REPEAT_EVERY_DAY;
/** @type {String} */
var MSG_REPEAT_EVERY_WEEK = MSG_REPEAT_EVERY_DAY;
/** @type {String} */
var MSG_REPEAT_EVERY_PLURAL_WEEK = MSG_REPEAT_EVERY_DAY;
/** @type {String} */
var MSG_REPEAT_EVERY_MONTH = MSG_REPEAT_EVERY_DAY;
/** @type {String} */
var MSG_REPEAT_EVERY_PLURAL_MONTH = MSG_REPEAT_EVERY_DAY;
/** @type {String} */
var MSG_REPEAT_EVERY_YEAR = MSG_REPEAT_EVERY_DAY;
/** @type {String} */
var MSG_REPEAT_EVERY_PLURAL_YEAR = MSG_REPEAT_EVERY_DAY;
/** @type {String} */
var MSG_REPEAT_ON = ('Repeat On:');
/** @type {String} */
var MSG_REPEAT_BY = ('Repeat By:');
/** @type {String} */
var MSG_DAY_OF_THE_WEEK = ('day of the week');
/** @type {String} */
var MSG_DAY_OF_THE_MONTH = ('day of the month');
/** @type {String} */
var MSG_RECURS_UNTIL = ('Until');
/** @type {String} */
var MSG_RECURS_COUNT_TIMES = ('times');
/** @return {String} */
function MSG_RECUR_ALLDAY(ph0, ph1, ph2) {
  return (ph0 + ' ' + ph1 + ', ' + ph2);
}
/** @return {String} */
function MSG_RECUR_AT_TIME(ph0, ph1, ph2, ph3) {
  return (ph0 + ' at ' + ph3 + ' ' + ph1 + ' ' + ph2);
}
/** @type {String} */
var MSG_RECUR_ONCE = ('Once');
/** @type {String} */
var MSG_RECUR_DAILY = ('Daily');
/** @return {String} */
function MSG_RECUR_N_DAILY(ph0) {
  return ('Every ' + ph0 + ' days');
}
/** @type {String} */
var MSG_RECUR_WEEKLY = ('Weekly');
/** @return {String} */
function MSG_RECUR_N_WEEKLY(ph0) {
  return ('Every ' + ph0 + ' weeks');
}
/** @return {String} */
function MSG_RECUR_WEEKLY_DOW(ph0) {
  return ('on ' + ph0);
}
/** @type {function} */
var MSG_RECUR_WEEKLY_PLURAL_DOW = MSG_RECUR_WEEKLY_DOW;
/** @type {String} */
var MSG_RECUR_MONTHLY = ('Monthly');
/** @return {String} */
function MSG_RECUR_N_MONTHLY(ph0) {
  return ('Every ' + ph0 + ' months');
}
/** @return {String} */
function MSG_RECUR_MONTHLY_POS_DAY(ph0) {
  return ('on day ' + ph0);
}
/** @return {String} */
function MSG_RECUR_MONTHLY_POS_DAYS(ph0) {
  return ('on days ' + ph0 + ' of the month');
}
/** @return {String} */
function MSG_RECUR_MONTHLY_NEG_DAY(ph0) {
  return (ph0 + ' days before the end of the month');
}
/** @type {String} */
var MSG_RECUR_MONTHLY_LAST_DAY = ('on the last day');
/** @return {String} */
function MSG_RECUR_MONTHLY_POS_WEEKS(ph0, ph1) {
  return ('on ' + ph0 + ' of weeks ' + ph1 + ' of the month');
}
/** @return {String} */
function MSG_RECUR_MONTHLY_FIRST_WEEK(ph0) {
  return ('on the first ' + ph0);
}
/** @return {String} */
function MSG_RECUR_MONTHLY_SECOND_WEEK(ph0) {
  return ('on the second ' + ph0);
}
/** @return {String} */
function MSG_RECUR_MONTHLY_THIRD_WEEK(ph0) {
  return ('on the third ' + ph0);
}
/** @return {String} */
function MSG_RECUR_MONTHLY_FOURTH_WEEK(ph0) {
  return ('on the fourth ' + ph0);
}
/** @return {String} */
function MSG_RECUR_MONTHLY_FIFTH_WEEK(ph0) {
  return ('on the fifth ' + ph0);
}
/** @return {String} */
function MSG_RECUR_MONTHLY_EVERY_WEEK(ph0) {
  return ('every ' + ph0);
}
/** @return {String} */
function MSG_RECUR_MONTHLY_NEG_WEEK(ph0, ph1) {
  return ('on ' + ph0 + ', ' + ph1 + ' weeks before the end of the month');
}
/** @return {String} */
function MSG_RECUR_MONTHLY_LAST_WEEK(ph0) {
  return ('on the last ' + ph0);
}
/** @type {String} */
var MSG_RECUR_ANNUALLY = ('Annually');
/** @return {String} */
function MSG_RECUR_N_ANNUALLY(ph0) {
  return ('Every ' + ph0 + ' years');
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_DAYS_OF_MONTHS(ph0, ph1) {
  return ('on days ' + ph0 + ' of ' + ph1);
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_DAY_OF_MONTHS(ph0, ph1) {
  return ('on day ' + ph0 + ' of ' + ph1);
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_DAY_OF_MONTH(ph0, ph1) {
  return ('on ' + ph1 + ' ' + ph0);
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_NEG_DAY_OF_MONTH(ph0, ph1) {
  return (ph0 + ' days before the end of ' + ph1);
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_LAST_DAY_OF_MONTH(ph0) {
  return ('on the last day of ' + ph0);
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_WEEKS_OF_YEAR(ph0, ph1) {
  return ('on ' + ph0 + ' of weeks ' + ph1 + ' of the year');
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_WEEK_OF_YEAR(ph0, ph1) {
  return ('on ' + ph0 + ' of week ' + ph1 + ' of the year');
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_WEEKS_OF_MONTHS(ph0, ph1, ph2) {
  return ('on ' + ph0 + ' of weeks ' + ph1 + ' in ' + ph2);
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_FIRST_WEEK_OF_MONTHS(ph0, ph1) {
  return ('on the first ' + ph0 + ' in ' + ph1);
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_SECOND_WEEK_OF_MONTHS(ph0, ph1) {
  return ('on the second ' + ph0 + ' in ' + ph1);
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_THIRD_WEEK_OF_MONTHS(ph0, ph1) {
  return ('on the third ' + ph0 + ' in ' + ph1);
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_FOURTH_WEEK_OF_MONTHS(ph0, ph1) {
  return ('on the fourth ' + ph0 + ' in ' + ph1);
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_FIFTH_WEEK_OF_MONTHS(ph0, ph1) {
  return ('on the fifth ' + ph0 + ' in ' + ph1);
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_EVERY_WEEK_OF_YEAR(ph0) {
  return ('on every ' + ph0 + ' of the year');
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_EVERY_WEEK_OF_MONTHS(ph0, ph1) {
  return ('on every ' + ph0 + ' in ' + ph1);
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_NEG_WEEK_OF_YEAR(ph0, ph1) {
  return ('on ' + ph0 + ', ' + ph1 + ' weeks before the end of the year');
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_LAST_WEEK_OF_YEAR(ph0) {
  return ('on the last ' + ph0 + ' of the year');
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_NEG_WEEK_OF_MONTHS(ph0, ph1, ph2) {
  return ('on ' + ph0 + ', ' + ph1 + ' weeks before the end of ' + ph2);
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_LAST_WEEK_OF_MONTHS(ph0, ph1) {
  return ('on the last ' + ph0 + ' in ' + ph1);
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_DAY_OF_YEAR(ph0) {
  return ('on day ' + ph0 + ' of the year');
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_DAYS_OF_YEAR(ph0) {
  return ('on days ' + ph0 + ' of the year');
}
/** @return {String} */
function MSG_RECUR_ANNUALLY_NEG_DAY_OF_YEAR(ph0) {
  return (ph0 + ' days before the end of the year');
}
/** @type {String} */
var MSG_RECUR_ANNUALLY_LAST_DAY_OF_YEAR = ('on the last day of the year');
/** @type {String} */
var MSG_RECUR_FOREVER = ('\046#32;');
/** @return {String} */
function MSG_RECUR_UNTIL(ph0) {
  return ('until ' + ph0);
}
/** @return {String} */
function MSG_RECUR_COUNT(ph0) {
  return (ph0 + ' times');
}
/** @type {String} */
var MSG_RECUR_PERIOD_DAY_SINGULAR = MSG_DAYSET_EVERY_DAY;
/** @type {String} */
var MSG_RECUR_PERIOD_DAY_PLURAL = ('days');
/** @type {String} */
var MSG_RECUR_PERIOD_WEEK_SINGULAR = ('week');
/** @type {String} */
var MSG_RECUR_PERIOD_WEEK_PLURAL = ('weeks');
/** @type {String} */
var MSG_RECUR_PERIOD_MONTH_SINGULAR = ('month');
/** @type {String} */
var MSG_RECUR_PERIOD_MONTH_PLURAL = ('months');
/** @type {String} */
var MSG_RECUR_PERIOD_YEAR_SINGULAR = ('year');
/** @type {String} */
var MSG_RECUR_PERIOD_YEAR_PLURAL = ('years');
/** @return {String} */
function MSG_RDATA_EXCEPT_EX_DATA(ph0, ph1) {
  return (ph0 + ' except ' + ph1);
}
/** @return {String} */
function MSG_RDATA_INCLUSION_LIST(ph0, ph1) {
  return (ph0 + ' and ' + ph1);
}
/** @return {String} */
function MSG_RDATA_EXCLUSION_LIST(ph0, ph1) {
  return (ph0 + ' or ' + ph1);
}
/** @type {String} */
var MSG_ORDINAL_1 = ('first');
/** @type {String} */
var MSG_ORDINAL_2 = ('second');
/** @type {String} */
var MSG_ORDINAL_3 = ('third');
/** @return {String} */
function MSG_VIEW_EVENT_TEXT(ph0) {
  return ('View your event at ' + ph0 + '.');
}
/** @return {String} */
function MSG_SMS_VERIFICATION_MESSAGE_FORMAT(ph0) {
  return ('Your Google Calendar verification code is ' + ph0 + '. Std charges may apply to receive messages. For info text HELP, or to cancel text STOP to 48368');
}
/** @return {String} */
function MSG_SMS_VERIFICATION_MESSAGE_FORMAT_NO_SHORTCODE(ph0) {
  return ('Your Google Calendar verification code is ' + ph0 + '. Std charges may apply to receive messages.');
}
/** @return {String} */
function MSG_SMS_VERIFICATION_MESSAGE_FORMAT_NO_CHARGES(ph0) {
  return ('Your Google Calendar verification code is ' + ph0 + '. For info text HELP, or to cancel text STOP to 48368');
}
/** @return {String} */
function MSG_SMS_VERIFICATION_MESSAGE_FORMAT_NO_SHORTCODE_NO_CHARGES(ph0) {
  return ('Your Google Calendar verification code is ' + ph0 + '.');
}
/** @type {String} */
var MSG_SMS_COMMAND_STOP = ('stop');
/** @type {String} */
var MSG_SMS_COMMAND_NEXT = ('next');
/** @type {String} */
var MSG_SMS_COMMAND_DAY = MSG_DAYSET_EVERY_DAY;
/** @type {String} */
var MSG_SMS_COMMAND_NEXT_DAY = ('nday');
/** @return {String} */
function MSG_MONTH_YEAR(ph0, ph1) {
  return (ph0 + ' ' + ph1);
}
/** @type {function} */
var MSG_MONTH_DAY = MSG_MONTH_YEAR;
/** @type {function} */
var MSG_MONTH_DAY_YEAR = MSG_RECUR_ALLDAY;
/** @return {String} */
function MSG_SAME_MONTH_DATERANGE(ph0, ph1, ph2, ph3) {
  return (ph0 + ' ' + ph1 + ' - ' + ph2 + ' ' + ph3);
}
/** @type {function} */
var MSG_WEEKDAY_DATE = MSG_MONTH_YEAR;
/** @return {String} */
function MSG_HOUR_AMPM(ph0, ph1) {
  return (ph0 + '' + ph1);
}
/** @return {String} */
function MSG_HOUR_MINUTE_AMPM(ph0, ph1, ph2) {
  return (ph0 + ':' + ph1 + ph2);
}
/** @return {String} */
function MSG_DATE_SEPARATOR(ph0, ph1) {
  return (ph0 + ' - ' + ph1);
}
/** @return {String} */
function MSG_YEAR_MONTH_DAY(ph0, ph1, ph2) {
  return (ph0 + '-' + ph1 + '-' + ph2);
}
/**
 * @type {String}
 */
var MSG_SU = 'S';
/**
 * @type {String}
 */
var MSG_M = 'M';
/**
 * @type {String}
 */
var MSG_TU = 'T';
/**
 * @type {String}
 */
var MSG_W = 'W';
/**
 * @type {String}
 */
var MSG_TH = 'T';
/**
 * @type {String}
 */
var MSG_F = 'F';
/**
 * @type {String}
 */
var MSG_SA = 'S';
/**
 * @type {String}
 */
var MSG_SUN = 'Sun';
/**
 * @type {String}
 */
var MSG_MON = 'Mon';
/**
 * @type {String}
 */
var MSG_TUES = 'Tue';
/**
 * @type {String}
 */
var MSG_WED = 'Wed';
/**
 * @type {String}
 */
var MSG_THUR = 'Thu';
/**
 * @type {String}
 */
var MSG_FRI = 'Fri';
/**
 * @type {String}
 */
var MSG_SAT = 'Sat';
/**
 * @type {String}
 */
var MSG_SUNDAY = 'Sunday';
/**
 * @type {String}
 */
var MSG_MONDAY = 'Monday';
/**
 * @type {String}
 */
var MSG_TUESDAY = 'Tuesday';
/**
 * @type {String}
 */
var MSG_WEDNESDAY = 'Wednesday';
/**
 * @type {String}
 */
var MSG_THURSDAY = 'Thursday';
/**
 * @type {String}
 */
var MSG_FRIDAY = 'Friday';
/**
 * @type {String}
 */
var MSG_SATURDAY = 'Saturday';
/**
 * @type {String}
 */
var MSG_JAN = 'Jan';
/**
 * @type {String}
 */
var MSG_FEB = 'Feb';
/**
 * @type {String}
 */
var MSG_MAR = 'Mar';
/**
 * @type {String}
 */
var MSG_APR = 'Apr';
/**
 * @type {String}
 */
var MSG_MAY = 'May';
/**
 * @type {String}
 */
var MSG_JUN = 'Jun';
/**
 * @type {String}
 */
var MSG_JUL = 'Jul';
/**
 * @type {String}
 */
var MSG_AUG = 'Aug';
/**
 * @type {String}
 */
var MSG_SEP = 'Sep';
/**
 * @type {String}
 */
var MSG_OCT = 'Oct';
/**
 * @type {String}
 */
var MSG_NOV = 'Nov';
/**
 * @type {String}
 */
var MSG_DEC = 'Dec';
/**
 * @type {String}
 */
var MSG_JANUARY = 'January';
/**
 * @type {String}
 */
var MSG_FEBRUARY = 'February';
/**
 * @type {String}
 */
var MSG_MARCH = 'March';
/**
 * @type {String}
 */
var MSG_APRIL = 'April';
/**
 * @type {String}
 */
var MSG_MAY_LONG = 'May';
/**
 * @type {String}
 */
var MSG_JUNE = 'June';
/**
 * @type {String}
 */
var MSG_JULY = 'July';
/**
 * @type {String}
 */
var MSG_AUGUST = 'August';
/**
 * @type {String}
 */
var MSG_SEPTEMBER = 'September';
/**
 * @type {String}
 */
var MSG_OCTOBER = 'October';
/**
 * @type {String}
 */
var MSG_NOVEMBER = 'November';
/**
 * @type {String}
 */
var MSG_DECEMBER = 'December';
/**
 * @type {String}
 */
var MSG_AM_FULL = 'am';
/**
 * @type {String}
 */
var MSG_PM_FULL = 'pm';
/**
 * @type {String}
 */
var MSG_AM = 'am';
/**
 * @type {String}
 */
var MSG_PM = 'pm';
/**
 * @type {String}
 */
var MSG_AM_SHORT = 'a';
/**
 * @type {String}
 */
var MSG_PM_SHORT = 'p';
/**
 * @type {Boolean}
 */
var USE_NUMERIC_MONTHS = false;
/**
 * The locale used to generate all the MSG_* constants
 * @type {String}
 */
var MESSAGE_LOCALE = 'en';
/**
 * The language for all the MSG_* constants
 * @type {String}
 */
var MESSAGE_LOCALE_LANG = 'en';
/**
 * Is the current locale Chinese, Japanese, or Korean?
 * @type {Boolean}
 */
var IS_CJK = false;
/**
 * @type {String}
 */
var GWS_MESSAGE_LOCALE = 'en';
/**
 * the list of supported locales
 * @type {Array}
 */
var LOCALES = ['en','zh_TW','zh_CN','da','nl','en_GB','fi','fr','de','it','ja','ko','no','pl','pt_BR','ru','es','sv','tr','en_US_pseudo'];
/**
 * @param {Number} hour
 * @param {Number} min
 * @return {String}
 */
function MSG_TIME24HM(hour, min) {
  var h = (undefined == hour) ? '??' : '' + hour;
  if (h.length < 2) { h = '0' + h; }
  var m = (undefined != min) ? (min < 10 ? '0' : '') + min : '??';
  return h + ':' + m;
}
/**
 * @param {Number} hour
 * @param {Number} min
 * @return {String}
 */
function MSG_TIME12HM(hour, min) {
  var ampm = hour < 12 ? MSG_AM : MSG_PM;
  var h = (undefined == hour) ? '??' : '' + ((hour % 12) || 12);
  var m = (undefined != min) ? (min < 10 ? '0' : '') + min : '??';
  return MSG_HOUR_MINUTE_AMPM(h, m, ampm);
}
/**
 * @param {Number} hour
 * @return {String}
 */
function MSG_TIME12H(hour) {
  var ampm = hour < 12 ? MSG_AM : MSG_PM;
  var h = (undefined == hour) ? '??' : '' + ((hour % 12) || 12);
  return MSG_HOUR_AMPM(h, ampm);
}
/**
 * @param {Number} hour
 * @param {Number} min
 * @return {String}
 */
function MSG_TIME12HM_ABBREV(hour, min) {
  var ampm = hour < 12 ? '' : MSG_PM_SHORT;
  var h = (undefined == hour) ? '??' : '' + ((hour % 12) || 12);
  var m = (undefined != min) ? (min < 10 ? '0' : '') + min : '??';
  return MSG_HOUR_MINUTE_AMPM(h, m, ampm);
}
/**
 * @param {Number} hour
 * @return {String}
 */
function MSG_TIME12H_ABBREV(hour) {
  var ampm = hour < 12 ? '' : MSG_PM_SHORT;
  var h = (undefined == hour) ? '??' : '' + ((hour % 12) || 12);
  return MSG_HOUR_AMPM(h, ampm);
}
/**
 * @param {Number} hour
 * @param {Number} min
 * @return {String}
 */
function MSG_TIME12HM_RAW(hour, min) {
  var h = (undefined == hour) ? '??' : '' + ((hour % 12) || 12);
  var m = (undefined != min) ? (min < 10 ? '0' : '') + min : '??';
  return MSG_HOUR_MINUTE_AMPM(h, m, '');
}
/**
 * @param {Number} hour
 * @return {String}
 */
function MSG_TIME12H_RAW(hour) {
  var h = (undefined == hour) ? '??' : '' + ((hour % 12) || 12);
  return MSG_HOUR_AMPM(h, '');
}
/**
 * @type {Boolean}
 */
var FEATURE_QUICKADD = true;
/**
 * @type {Boolean}
 */
var FEATURE_GOOGLE_MAPS_LINKS = true;
/**
 * @type {Boolean}
 */
var FEATURE_EXTRA_CCC_LINKS = true;
/**
 * @type {Boolean}
 */
var FEATURE_EVENT_SEARCH = true;
