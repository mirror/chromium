// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


'use strict';


/**
 * Alias for document.getElementById.
 * @param {string} id The ID of the element to find.
 * @return {HTMLElement} The found element or null if not found.
 */
function $(id) {
  // eslint-disable-next-line no-restricted-properties
  return document.getElementById(id);
}

var sfs = {};
sfs.speech = {};
sfs.view = {};

sfs.EventType = {
  MOUSEUP: 'mouseup'
};


/**
 * Enum for keycodes.
 * @enum {number}
 * @const
 */
var KEYCODE = {ENTER: 13, ESC: 27, PERIOD: 190};


/* GLOBALS */
/**
 * @fileoverview Contains constants used in Speech.
 */


/**
 * The set of possible recognition errors.
 * @enum {string}
 */
sfs.speech.RecognitionError = {
  NO_SPEECH: '0',
  ABORTED: '1',
  AUDIO_CAPTURE: '2',
  NETWORK: '3',
  NOT_ALLOWED: '4',
  SERVICE_NOT_ALLOWED: '5',
  BAD_GRAMMAR: '6',
  LANGUAGE_NOT_SUPPORTED: '7',
  NO_MATCH: '8',
  OTHER: '9'
};


/**
 * Localized translations for messages used in the Speech UI.
 * @type {{
 *   audioError: string,
 *   languageError: string,
 *   listening: string,
 *   networkError: string,
 *   noTranslation: string,
 *   noVoice: string,
 *   permissionError: string,
 *   ready: string,
 *   waiting: string
 * }}
 */
sfs.speech.messages = {
  audioError: '',
  languageError: '',
  listening: '',
  networkError: '',
  noTranslation: '',
  noVoice: '',
  permissionError: '',
  ready: '',
  waiting: ''
};
/* GLOBALS END */

/**
 * The set of reasons the controller generates logs.
 * @enum {string}
 * @private
 */
sfs.speech.LogReason_ = {
  ESCAPE_KEY: '0',
  WEBKIT_SPEECH_RECOGNITION: '1',
  UPDATE_LOCATION: '2',
  IDLE_TIMEOUT: '3',
  VISIBILITY_CHANGE: '4',
  NOT_READY: '5',
  PERMISSION_TIMEOUT: '6',
  FOCUS_CHANGE: '7'
};


/**
 * The sources of logs, passed to the gen_204 'cad' param, to indicate what
 * the logs were issued for.
 * @enum {number}
 * @private
 */
sfs.speech.LogSource_ = {
  CONTROLLER: 0,
  CLICK_ABORT: 1,
  API_ERROR: 2,
  CONTROLLER_ERROR: 3,
  NO_MATCH: 4,
  DISABLED: 5,
  RETRY_CLICK: 6  //,
  // Triggered by voice via Chrome Hotword extension.
  // HOTWORD_VOICE_TRIGGER: 7
};


/**
 * The set of methods that can trigger a search action.
 * TODO(melevin): Resolve having to sync this with:
 * searchbox/javascript/shared/src/shared_constants.js
 * @enum {number}
 * @private
 */
sfs.speech.SearchMethod_ = {
  ENTER_KEY: 3,
  CLICK: 17,
  FINAL_RESULT: 18,
  IDLE_TIMEOUT: 19,
  RESTORE: 20
};


/**
 * Messages used to communicate with the Chrome Hotword extension regarding
 * the state of the Speech UI.
 * @enum {string}
 * @private
 */
/*
sfs.speech.HotwordMessage_ = {
  SPEECH_START: 'ss',
  SPEECH_RESET: 'sr',
  SPEECH_ERROR: 'se'
};
*/


/**
 * The set of controller states.
 * @enum {number}
 * @private
 */
sfs.speech.State_ = {
  DISABLED: -1,
  READY: 0,
  INITIALIZING: 1,
  STARTING: 2,
  STARTED: 3,
  AUDIO_RECEIVED: 4,
  SPEECH_RECEIVED: 5,
  RESULT_RECEIVED: 6,
  SEARCH_SUBMITTED: 7,
  ERROR_RECEIVED: 8,
  ERROR_FORCED: 9,
  ABORTED: 10,
  STOPPED: 11
};

/* @const */ var speech = sfs.speech;
/* @const */ var view = sfs.view;
// /* @const */ var HotwordUi = gws.embeddedsearch.HotwordUi;
/* @const */ var RecognitionError = sfs.speech.RecognitionError;
/* @const */ var LogReason_ = sfs.speech.LogReason_;
/* @const */ var LogSource_ = sfs.speech.LogSource_;
/* @const */ var SearchMethod_ = sfs.speech.SearchMethod_;
/* @const */ var State_ = sfs.speech.State_;


/**
 * Time in milliseconds to wait before closing the UI after an error has
 * occured. This is a short timeout used when no click-target is present.
 * @private {number}
 * @const
 */
sfs.speech.ERROR_TIMEOUT_SHORT_MS_ = 3000;


/**
 * Time in milliseconds to wait before closing the UI after an error has
 * occured. This is a longer timeout used when there is a click-target is
 * present.
 * @private {number}
 * @const
 */
sfs.speech.ERROR_TIMEOUT_LONG_MS_ = 8000;


/**
 * Time in milliseconds to wait before closing the UI if no interaction has
 * occured.
 * @private {number}
 * @const
 */
sfs.speech.IDLE_TIMEOUT_MS_ = 8000;


/**
 * Time in milliseconds to wait before closing the UI if no interaction has
 * occured when the permissions bar is displayed to the user.
 * @private {number}
 * @const
 */
sfs.speech.PERMISSION_IDLE_TIMEOUT_MS_ = 15000;


/**
 * Time in milliseconds to wait for audio input to start before assuming we
 * are waiting for the user to give permission.
 * @private {number}
 * @const
 */
sfs.speech.PERMISSION_TIMEOUT_MS_ = 1500;


/**
 * The 'ct' param on the gen_204. This param identifies the product that is
 * doing the logging.
 * @private {string}
 * @const
 */
sfs.speech.SPEECH_CT_ = 'spch-recog';


/**
 * Specifies the current state of the controller.
 * @private {State_}
 */
sfs.speech.currentState_ = State_.DISABLED;


/**
 * The ID for the error timer.
 * @private {number}
 */
sfs.speech.errorTimer_;


/**
 * The duration of the timeout for the UI elements during an error state.
 * Depending on the error state, we have different durations for the timeout.
 * @private {number}
 */
sfs.speech.errorTimeoutMs_ = 0;


/**
 * The last high confidence voice transcript received.
 * @private {string}
 */
sfs.speech.finalResult_;


/**
 * The ID for the idle timer.
 * @private {number}
 */
sfs.speech.idleTimer_;


/**
 * The last low confidence voice transcript received.
 * @private {string}
 */
sfs.speech.interimResult_;


/**
 * The language to perform the speech recognition in.
 * @private {string}
 */
sfs.speech.language_;


/**
 * The ID for the permission timer.
 * @private {number}
 */
sfs.speech.permissionTimer_;


/**
 * The Web Speech API element driving the whole process.
 * @private {!webkitSpeechRecognition}
 */
sfs.speech.recognition_;


/**
 * The query used to fetch the last set of speech results.
 * @private {string}
 */
sfs.speech.speechQuery_;


/**
 * HotwordUi object used to interact with hotword ui components.
 * @private {HotwordUi}
 */
// sfs.speech.hotwordUi_;


/**
 * Initializes the speech module.
 * @param {!Object} configData The NTP configuration.
 * @private
 * @suppress {const}
 */
sfs.speech.init_ = function(configData) {
  speech.language_ = configData.language;
  if (speech.isRecognitionAvailable_() && view.init(speech.handleClickEvent_)) {
    console.log('Speech is available, initializing...');
    if (speech.currentState_ == State_.DISABLED) {
      // Add listeners if the speech module is initializing for the first time
      // or if it was previously disabled.

      // TODO(oskopek): Do we need this at all?
      /*
      // Listen for the keyboard shortcut trigger.
      goog.events.listen(window, 'keydown', speech.handleKeyDown_);

      // Listen for the Searchbox microphone icon click.
      gws.msg.listen(
          gws.constants.MESSAGE_TYPE_SEARCHBOX_SPEECH_BUTTON_CLICKED,
          speech.toggleStartStop_);

      // Listen for the Fakebox microphone icon click.
      gws.msg.listen(
          gws.constants.MESSAGE_TYPE_EMBEDDED_SEARCH_SPEECH_BUTTON_CLICKED,
          speech.toggleStartStop_);

      // Listen for the omnibox getting focus.
      gws.msg.listen(
          gws.constants.MESSAGE_TYPE_EMBEDDED_SEARCH_OMNIBOX_FOCUSED,
          speech.handleEmbeddedSearchOmniboxFocused_);
      */

      // Listen for the hotword voice trigger to start speech input.
      /*gws.msg.listen(
          gws.constants.MESSAGE_TYPE_HOTWORD_VOICE_TRIGGER,
          speech.handleHotwordVoiceTrigger_);
      */
    }

    // TODO(dou): Refactor this code so that hotwordui is in its own
    // plugin and initializes itself instead of being initialized here
    // as part of the speech module.
    if (configData.isHotwordEnabled) {
      // Creating a new instance of hotword here.
      speech.hotwordUi_ = HotwordUi.getInstance();
      speech.hotwordUi_.setHotwordTooltipMessage(
          configData.translatedStrings.hotwordTooltip);
      speech.hotwordUi_.wakeUpHelper();
    }

    // Set translations map.
    sfs.speech.messages = {
      listening: configData.translatedStrings.listening,
      ready: configData.translatedStrings.ready,
      noTranslation: configData.translatedStrings.noTranslation,
      waiting: configData.translatedStrings.waiting,
      audioError: configData.translatedStrings.audioError,
      networkError: configData.translatedStrings.networkError,
      languageError: configData.translatedStrings.languageError,
      permissionError: configData.translatedStrings.permissionError,
      noVoice: configData.translatedStrings.noVoice
    };

    speech.currentState_ = State_.INITIALIZING;
    speech.initWebkitSpeech_();
    speech.reset_();
  } else {
    console.log('Speech is unavailable, disabling.');
    speech.disable_();
  }
};


/**
 * Disables the speech recognition interface.
 * @private
 */
sfs.speech.disable_ = function() {
  speech.currentState_ = State_.DISABLED;
  // Send a message to Searchbox thas speech recognition is disabled.
  // TODO(oskopek): Do we need this?
  /*
  gws.msg.send(gws.constants.MESSAGE_TYPE_SPEECH_RECOGNITION_DISABLED);
  goog.events.unlisten(window, 'keydown', speech.handleKeyDown_);

  gws.msg.unlisten(
      gws.constants.MESSAGE_TYPE_SEARCHBOX_SPEECH_BUTTON_CLICKED,
      speech.toggleStartStop_);

  gws.msg.unlisten(
      gws.constants.MESSAGE_TYPE_EMBEDDED_SEARCH_SPEECH_BUTTON_CLICKED,
      speech.toggleStartStop_);

  gws.msg.unlisten(
      gws.constants.MESSAGE_TYPE_EMBEDDED_SEARCH_OMNIBOX_FOCUSED,
      speech.handleEmbeddedSearchOmniboxFocused_);
  */

  /*
  gws.msg.unlisten(
      gws.constants.MESSAGE_TYPE_HOTWORD_VOICE_TRIGGER,
      speech.handleHotwordVoiceTrigger_);
  */
};


/**
 * Initializes the voice search element.
 * @private
 */
sfs.speech.initWebkitSpeech_ = function() {
  speech.recognition_ = new webkitSpeechRecognition();
  speech.recognition_.continuous = false;
  speech.recognition_.interimResults = true;
  speech.recognition_.lang = speech.language_;
  speech.recognition_.maxAlternatives = 4;
  speech.recognition_.onerror = speech.handleRecognitionError_;
  speech.recognition_.onnomatch = speech.handleRecognitionOnNoMatch_;
  speech.recognition_.onend = speech.handleRecognitionEnd_;
  speech.recognition_.onresult = speech.handleRecognitionResult_;
  speech.recognition_.onaudiostart = speech.handleRecognitionAudioStart_;
  speech.recognition_.onspeechstart = speech.handleRecognitionSpeechStart_;
};


/**
 * Sets up the necessary states for voice search and then starts the
 * speech recognition interface.
 * @private
 */
sfs.speech.start_ = function() {
  if (speech.currentState_ == State_.STARTING) {
    // speech.postWindowMessage_(speech.HotwordMessage_.SPEECH_START);

    sfs.view.show();

    speech.resetIdleTimer_(speech.IDLE_TIMEOUT_MS_);
    speech.resetPermissionTimer_();

    // Send a message to Searchbox and Psychic that speech recognition input
    // is about to start.
    // TODO(oskopek): Do we need this?
    // gws.msg.send(gws.constants.MESSAGE_TYPE_BEFORE_SPEECH_RECOGNITION_START);


    document.addEventListener(
        'webkitvisibilitychange', speech.handleVisibilityChange_, false);

    if (!speech.isRecognitionInitialized_()) {
      speech.initWebkitSpeech_();
    }

    // If speech.start_() is called too soon after speech.stop_() then the
    // recognition interface hasn't yet reset and an error occurs. In this case
    // we need to hard-reset it and reissue the start() command.
    try {
      speech.recognition_.start();
      speech.currentState_ = State_.STARTED;
    } catch (error) {
      speech.currentState_ = State_.STARTING;
      speech.initWebkitSpeech_();
      try {
        speech.recognition_.start();
        speech.currentState_ = State_.STARTED;
      } catch (error2) {
        speech.currentState_ = State_.ABORTED;
        speech.logGen204_(
            LogSource_.CONTROLLER, LogReason_.WEBKIT_SPEECH_RECOGNITION);
        speech.stop_();
      }
    }
  }
};


/**
 * Stops the speech recognition interface and restores the input field value.
 * @private
 */
sfs.speech.stop_ = function() {
  if (speech.currentState_ != State_.SEARCH_SUBMITTED) {
    // Go back to Psychic from Instant Gratification and notify Searchbox.
    // TODO(oskopek): Do we need this?
    // gws.msg.send(gws.constants.MESSAGE_TYPE_AFTER_SPEECH_RECOGNITION_ABORT);
  }
  if (speech.currentState_ == State_.ABORTED) {
    speech.currentState_ = State_.STOPPED;
    speech.recognition_.abort();
  }
  sfs.view.hide();
  speech.reset_();
};


/**
 * Aborts the speech recognition interface and logs the abortion.
 * @param {LogSource_} abortedBy The source of the abort.
 * @param {!LogReason_|string} reason The reason for the abort.
 * @param {string=} opt_additionalData Any additional data to be logged.
 * @private
 */
sfs.speech.abort_ = function(abortedBy, reason, opt_additionalData) {
  speech.logGen204_(abortedBy, reason, opt_additionalData);
  speech.currentState_ = State_.ABORTED;
  speech.stop_();
};


/**
 * Resets the speech recognition interface.
 * We don't reset logging here as we mostly call this function when the user
 * aborts speech recognition or there's an error. Logging is reset only when
 * there is a new Searchbox session, after {@code speech.submitSpeechQuery_()}.
 * @private
 */
sfs.speech.reset_ = function() {
  window.clearTimeout(speech.idleTimer_);
  window.clearTimeout(speech.errorTimer_);
  window.clearTimeout(speech.permissionTimer_);

  // Notify Searchbox that Speech is being reset.
  // TODO(oskopek): Do we need this?
  // gws.msg.send(gws.constants.MESSAGE_TYPE_SPEECH_RESET);

  // Re-enable the HP->SERP transition.
  /* TODO(oskopek): Do we need this?
  gws.msg.unlisten(
      gws.constants.MESSAGE_TYPE_PSYCHIC_BEFORE_CLEAR_HOME_PAGE,
      speech.handleOnBeforePsychicHomePageClear_, 50);
  */

  document.removeEventListener(
      'webkitvisibilitychange', speech.handleVisibilityChange_, false);

  // speech.postWindowMessage_(speech.HotwordMessage_.SPEECH_RESET);
  speech.speechQuery_ = '';
  speech.interimResult_ = '';
  speech.finalResult_ = '';
  speech.currentState_ = State_.READY;
  speech.recognition_.abort();
};


/**
 * Informs the view that the browser is receiving audio input.
 * @param {Event=} opt_event Emitted event for audio start.
 * @private
 */
sfs.speech.handleRecognitionAudioStart_ = function(opt_event) {
  speech.resetIdleTimer_(speech.IDLE_TIMEOUT_MS_);
  window.clearTimeout(speech.permissionTimer_);
  speech.currentState_ = State_.AUDIO_RECEIVED;
  sfs.view.setReadyForSpeech();
};


/**
 * Function is called when the user starts speaking.
 * @param {Event=} opt_event Emitted event for speech start.
 * @private
 */
sfs.speech.handleRecognitionSpeechStart_ = function(opt_event) {
  speech.resetIdleTimer_(speech.IDLE_TIMEOUT_MS_);
  speech.currentState_ = State_.SPEECH_RECEIVED;
  sfs.view.setReceivingSpeech();

  // Although fallBackToIG is set in Psychic by sending the message
  // MESSAGE_TYPE_BEFORE_SPEECH_START, the Searchbox is still moved with
  // simulateUserInput, so we stop it.
  /* TODO(oskopek): Do we need this?
  gws.msg.listen(
      gws.constants.MESSAGE_TYPE_PSYCHIC_BEFORE_CLEAR_HOME_PAGE,
      speech.handleOnBeforePsychicHomePageClear_, 50);
  */
};


/**
 * Processes the recognition results arriving from the Web Speech API.
 * @param {SpeechRecognitionEvent} responseEvent Event coming from the API.
 * @private
 */
sfs.speech.handleRecognitionResult_ = function(responseEvent) {
  speech.resetIdleTimer_(speech.IDLE_TIMEOUT_MS_);

  // We only want to process results in certain cases. The first two are
  // 'normal' operations. The last two cases are if there is some network
  // bugginess and the onaudiostart and onspeechstart packets were lost.
  switch (speech.currentState_) {
    case State_.RESULT_RECEIVED:
    case State_.SPEECH_RECEIVED:
      break;
    case State_.AUDIO_RECEIVED:
      speech.handleRecognitionSpeechStart_();
      break;
    case State_.STARTED:
      speech.handleRecognitionSpeechStart_();
      speech.handleRecognitionAudioStart_();
      break;
    default:
      return;
  }

  /* @const */ var results = responseEvent.results;
  /* @const */ var resultsLength = results.length;
  if (resultsLength == 0) {
    return;
  }
  speech.currentState_ = State_.RESULT_RECEIVED;
  speech.interimResult_ = '';
  speech.finalResult_ = '';
  /* @const */ var i = responseEvent.resultIndex;
  /* @const */ var isFinalResult = results[i].isFinal;
  if (!isFinalResult) {
    for (let j = 0; j < resultsLength; j++) {
      /* @const */ var resultTranscript = results[j][0].transcript;
      speech.interimResult_ += resultTranscript;
      if (results[j][0].confidence > 0.5) {
        speech.finalResult_ += resultTranscript;
      }
    }
    sfs.view.updateSpeechResult(speech.interimResult_, speech.finalResult_);
  } else {
    speech.finalResult_ = results[i][0].transcript;
    sfs.view.updateSpeechResult(speech.finalResult_, speech.finalResult_);
  }

  // If the user aborts, we want to not issue any queries.
  if (speech.currentState_ != State_.RESULT_RECEIVED) {
    return;
  }

  // Force-stop long queries.
  if (isFinalResult || speech.interimResult_.length > 120) {
    speech.speechQuery_ = speech.finalResult_;
    speech.submitSpeechQuery_(SearchMethod_.FINAL_RESULT);
  } else {
    speech.speechQuery_ = speech.interimResult_;

    // Communicate the interim result back to Searchbox.
    /* TODO(oskopek): Do we need this?
    gws.msg.send(
        gws.constants.MESSAGE_TYPE_SPEECH_RECOGNITION_RESULT,
        [speech.speechQuery_]);
    */
  }
};


/**
 * Called when an error from Web Speech API is received.
 * @param {SpeechRecognitionError} error The error event.
 * @private
 */
sfs.speech.handleRecognitionError_ = function(error) {
  speech.resetIdleTimer_(speech.IDLE_TIMEOUT_MS_);
  window.clearTimeout(speech.permissionTimer_);
  /* @const */ var speechError = speech.getRecognitionError_(error.error);
  if (speechError != RecognitionError.ABORTED) {
    let additionalData = '';
    if (speechError == RecognitionError.OTHER) {
      additionalData = error.error;
    }
    speech.logGen204_(LogSource_.API_ERROR, speechError, additionalData);
    speech.currentState_ = State_.ERROR_RECEIVED;
    sfs.view.showError(speechError);
    // speech.postWindowMessage_(speech.HotwordMessage_.SPEECH_ERROR);
    window.clearTimeout(speech.idleTimer_);
    speech.resetErrorTimer_(speech.errorTimeoutMs_);
  }
};


/**
 * Stops speech recognition when no matches are found.
 * @private
 */
sfs.speech.handleRecognitionOnNoMatch_ = function() {
  speech.logGen204_(LogSource_.NO_MATCH);
  speech.currentState_ = State_.ERROR_RECEIVED;
  sfs.view.showError(RecognitionError.NO_MATCH);
  // speech.postWindowMessage_(speech.HotwordMessage_.SPEECH_ERROR);
  window.clearTimeout(speech.idleTimer_);
  speech.resetErrorTimer_(speech.ERROR_TIMEOUT_LONG_MS_);
};


/**
 * Stops the UI when the Web Speech API reports that it has halted speech
 * recognition.
 * @private
 */
sfs.speech.handleRecognitionEnd_ = function() {
  window.clearTimeout(speech.idleTimer_);
  window.clearTimeout(speech.permissionTimer_);
  let error = RecognitionError.OTHER;
  switch (speech.currentState_) {
    case State_.STARTED:
      speech.currentState_ = State_.ERROR_FORCED;
      error = RecognitionError.AUDIO_CAPTURE;
      break;
    case State_.AUDIO_RECEIVED:
      speech.currentState_ = State_.ERROR_FORCED;
      error = RecognitionError.NO_SPEECH;
      break;
    case State_.SPEECH_RECEIVED:
    case State_.RESULT_RECEIVED:
      speech.currentState_ = State_.ERROR_FORCED;
      error = RecognitionError.NO_MATCH;
      break;
    case State_.ERROR_RECEIVED:
      break;
    default:
      return;
  }

  switch (speech.currentState_) {
    case State_.ERROR_FORCED:
      sfs.view.showError(error);
      // speech.postWindowMessage_(speech.HotwordMessage_.SPEECH_ERROR);
      speech.resetErrorTimer_(speech.ERROR_TIMEOUT_LONG_MS_);
      speech.logGen204_(LogSource_.CONTROLLER_ERROR, error);
    case State_.ERROR_RECEIVED:
      speech.currentState_ = State_.STOPPED;
      break;
    default:
      speech.currentState_ = State_.STOPPED;
      speech.stop_();
      break;
  }
};


/**
 * Handles the following keyboard actions.
 * - <CTRL> + <SHIFT> + <.> starts voice input(<CMD> + <SHIFT> + <.> on mac).
 * - <ESC> aborts voice input when the recognition interface is active.
 * - <ENTER> submits the speech query if there is one.
 * @param {!goog.events.BrowserEvent} event The keydown event.
 * @private
 */
sfs.speech.handleKeyDown_ = function(event) {
  function isUserAgentMac(userAgent) {
    return userAgent.includes('Macintosh');
  }

  if (!speech.isRecognizing_()) {
    /* @const */ var ctrlKeyPressed = event.ctrlKey ||
        (isUserAgentMac(window.navigator.userAgent) && event.metaKey);
    if (speech.currentState_ == State_.READY &&
        event.keyCode == KEYCODE.PERIOD && event.shiftKey && ctrlKeyPressed) {
      speech.toggleStartStop_();
    }
    return;
  }

  // Ensures that keyboard events are not propagated to the Searchbox client
  // during voice input.
  event.stopPropagation();
  if (event.keyCode == KEYCODE.ESC) {
    speech.abort_(LogSource_.CONTROLLER, LogReason_.ESCAPE_KEY);
  } else if (event.keyCode == KEYCODE.ENTER && speech.speechQuery_) {
    speech.submitSpeechQuery_(SearchMethod_.ENTER_KEY);
  }
};


/**
 * Handles the message sent by JESR that the page's hash has changed,
 * indicating that speech recognition should be aborted.
 * @return {boolean} True if speech recognition should be aborted.
 * @private
 */
sfs.speech.handleOnHashChange_ = function() {
  return true;
};


/**
 * Posts a message to the window to update the status of the speech query.
 * This is used by the Chrome Hotword extension.
 * http://go/ok-google
 * @param {string} message Message to be sent.
 * @private
 */
/*
sfs.speech.postWindowMessage_ = function(message) {
  window.postMessage({type: message}, '*');
};
*/


/**
 * Stops the HP->SERP transition for interim results. This way, when the user
 * cancels a voice search before a final result is received, they return
 * exactly to the state they started from.
 * @return {boolean} False if the transition should be stopped.
 * @private
 */
sfs.speech.handleOnBeforePsychicHomePageClear_ = function() {
  return speech.currentState_ == State_.SEARCH_SUBMITTED;
};


/**
 * Stops the recognition interface and closes the UI if no interactions occur
 * after some time and the interface is still active. This is a safety net in
 * case the recognition.onend event doesn't fire, as is sometime the case. If
 * a high confidence transcription was received then show the search results.
 * @private
 */
sfs.speech.handleIdleTimeout_ = function() {
  if (speech.finalResult_ != '') {
    speech.speechQuery_ = speech.finalResult_;
    speech.submitSpeechQuery_(SearchMethod_.IDLE_TIMEOUT);
    return;
  }
  switch (speech.currentState_) {
    case State_.STARTED:
    case State_.AUDIO_RECEIVED:
    case State_.SPEECH_RECEIVED:
    case State_.RESULT_RECEIVED:
    case State_.ERROR_RECEIVED:
      speech.abort_(LogSource_.CONTROLLER, LogReason_.IDLE_TIMEOUT);
      break;
  }
};


/**
 * Indicates that the Web Speech API is waiting for permission to use the
 * microphone.
 * @private
 */
sfs.speech.handlePermissionTimeout_ = function() {
  speech.resetIdleTimer_(speech.PERMISSION_IDLE_TIMEOUT_MS_);
  speech.logGen204_(LogSource_.CONTROLLER, LogReason_.PERMISSION_TIMEOUT);
  sfs.view.setWaitingForPermission();
};


/**
 * Aborts the speech recognition interface when the user switches to a new
 * tab or window.
 * @private
 */
sfs.speech.handleVisibilityChange_ = function() {
  if (speech.isUiHidden_()) {
    return;
  }

  // Do not write this as document.webkitHidden, as the JS compiler renames
  // "webkitHidden" to something else, and this check will thus always be
  // false.
  if (document['webkitHidden']) {
    speech.abort_(LogSource_.CONTROLLER, LogReason_.VISIBILITY_CHANGE);
  }
};


/**
 * Aborts the speech session if the UI is showing and omnibox gets focused.
 * @private
 */
sfs.speech.handleEmbeddedSearchOmniboxFocused_ = function() {
  if (!speech.isUiHidden_()) {
    speech.abort_(LogSource_.CONTROLLER, LogReason_.FOCUS_CHANGE);
  }
};


/**
 * Submits the spoken speech query to perform a search on.
 * @param {SearchMethod_} searchMethod The method by which the
 *     speech query is being submitted.
 * @private
 */
sfs.speech.submitSpeechQuery_ = function(searchMethod) {
  speech.currentState_ = State_.SEARCH_SUBMITTED;
  window.clearTimeout(speech.idleTimer_);

  // Send out a message to either go back to Psychic from IG and/or perform
  // a search navigation.
  // TODO(oskopek): Send query.
  /*
  gws.msg.send(
      gws.constants.MESSAGE_TYPE_AFTER_SPEECH_RECOGNITION_END,
      [speech.speechQuery_, searchMethod]);
  */
  speech.stop_();
};


/**
 * Returns the error type based on the error string received from the webkit
 * speech recognition API.
 * @param {string} error The error string received from the webkit speech
 *     recognition API.
 * @return {RecognitionError} The appropriate error
 *     state from the RecognitionError enum.
 * @private
 */
sfs.speech.getRecognitionError_ = function(error) {
  switch (error) {
    case 'no-speech':
      speech.errorTimeoutMs_ = speech.ERROR_TIMEOUT_LONG_MS_;
      return RecognitionError.NO_SPEECH;
    case 'aborted':
      speech.errorTimeoutMs_ = speech.ERROR_TIMEOUT_SHORT_MS_;
      return RecognitionError.ABORTED;
    case 'audio-capture':
      speech.errorTimeoutMs_ = speech.ERROR_TIMEOUT_LONG_MS_;
      return RecognitionError.AUDIO_CAPTURE;
    case 'network':
      speech.errorTimeoutMs_ = speech.ERROR_TIMEOUT_SHORT_MS_;
      return RecognitionError.NETWORK;
    case 'not-allowed':
      speech.errorTimeoutMs_ = speech.ERROR_TIMEOUT_LONG_MS_;
      return RecognitionError.NOT_ALLOWED;
    case 'service-not-allowed':
      speech.errorTimeoutMs_ = speech.ERROR_TIMEOUT_LONG_MS_;
      return RecognitionError.SERVICE_NOT_ALLOWED;
    case 'bad-grammar':
      speech.errorTimeoutMs_ = speech.ERROR_TIMEOUT_SHORT_MS_;
      return RecognitionError.BAD_GRAMMAR;
    case 'language-not-supported':
      speech.errorTimeoutMs_ = speech.ERROR_TIMEOUT_SHORT_MS_;
      return RecognitionError.LANGUAGE_NOT_SUPPORTED;
    default:
      speech.errorTimeoutMs_ = speech.ERROR_TIMEOUT_SHORT_MS_;
      return RecognitionError.OTHER;
  }
};


/**
 * Logs the gen_204 request.
 * @param {LogSource_} cad The 'cad' parameter indicating the source of
 *     the gen_204 log.
 * @param {string=} opt_reason The reason for which the source generated the
 *     log.
 * @param {string=} opt_additionalData Any additional data that needs to be
 *     logged.
 * @private
 */
sfs.speech.logGen204_ = function(cad, opt_reason, opt_additionalData) {
  var params = '';
  if (opt_reason) {
    params += '&reason=' + opt_reason;
  }
  if (opt_additionalData) {
    params += '&data=' + opt_additionalData;
  }
  // TODO(oskopek): Logging.
  // console.log(speech.SPEECH_CT_ + ': ' + cad + params);

  // TODO(oskopek): Remove me.
  function findInverseString(obj, value) {
    var keys = Object.keys(obj);
    for (var index in keys) {
      var key = keys[index];
      if (obj[key] === value) {
        return key.toString();
      }
    }
    return undefined;
  }

  var logSource = findInverseString(sfs.speech.LogSource_, cad);
  var logReason = findInverseString(sfs.speech.LogReason_, opt_reason);
  console.log(
      `[${speech.SPEECH_CT_}]: Source: ${logSource} ` +
      `(${opt_reason}/${logReason}).` +
      (opt_additionalData ? ` Data: ${opt_additionalData}` : ''));
};


/**
 * Sets the permission timeout to indicate if the Web Speech API is waiting
 * for permission to use the microphone. It is assumed that if audio input
 * does not start before speech.PERMISSION_TIMEOUT_MS_ that the user has not yet
 * granted the browser permission to use the microphone.
 * @private
 */
sfs.speech.resetPermissionTimer_ = function() {
  window.clearTimeout(speech.permissionTimer_);
  speech.permissionTimer_ = window.setTimeout(
      speech.handlePermissionTimeout_, speech.PERMISSION_TIMEOUT_MS_);
};


/**
 * Resets the idle state timeout.
 * @param {number} duration The duration after which to close the UI.
 * @private
 */
sfs.speech.resetIdleTimer_ = function(duration) {
  window.clearTimeout(speech.idleTimer_);
  speech.idleTimer_ = window.setTimeout(speech.handleIdleTimeout_, duration);
};


/**
 * Resets the idle error state timeout.
 * @param {number} duration The duration after which to close the UI during an
 *     error state.
 * @private
 */
sfs.speech.resetErrorTimer_ = function(duration) {
  window.clearTimeout(speech.errorTimer_);
  speech.errorTimer_ = window.setTimeout(speech.stop_, duration);
};


/**
 * Check to see if the speech recognition interface is running.
 * @return {boolean} True, if the speech recognition interface is running.
 * @private
 */
sfs.speech.isRecognizing_ = function() {
  switch (speech.currentState_) {
    case State_.READY:
    case State_.ABORTED:
    case State_.STOPPED:
    case State_.ERROR_RECEIVED:
    case State_.DISABLED:
    case State_.INITIALIZING:
      return false;
  }
  return true;
};


/**
 * Check if the controller is in a state where the UI is definitely hidden.
 * Since we show the UI for a few seconds after we receive an error from the
 * API, we need a separate definition to isRecognizing to indicate when the UI
 * is hidden (b/9426349).
 * @return {boolean} True if the UI is hidden.
 * @private
 */
sfs.speech.isUiHidden_ = function() {
  switch (speech.currentState_) {
    case State_.READY:
    case State_.DISABLED:
    case State_.INITIALIZING:
      return true;
  }
  return false;
};


/**
 * Check if the Web Speech API is initialized and event functions are set.
 * Checks that the API event handlers are still set as there are cases that
 * the speech.recognition_ element is reset by Chrome, in which case
 * !!speech.recognition_ is true but the settings in speech.initWebkitSpeech_()
 * are lost.
 * @return {boolean} True if recognition element handlers are initialized.
 * @private
 */
sfs.speech.isRecognitionInitialized_ = function() {
  return !!speech.recognition_ && !!speech.recognition_.onerror &&
      !!speech.recognition_.onnomatch && !!speech.recognition_.onend &&
      !!speech.recognition_.onresult && !!speech.recognition_.onaudiostart &&
      !!speech.recognition_.onspeechstart;
};


/**
 * Check if the Web Speech API is available.
 * @return {boolean} True if the Web Speech API is available.
 * @private
 */
sfs.speech.isRecognitionAvailable_ = function() {
  return 'webkitSpeechRecognition' in self && !!webkitSpeechRecognition;
};


/**
 * A voice search is triggered by hotword.
 * @private
 */
/*
 sfs.speech.handleHotwordVoiceTrigger_ = function() {
  if (speech.currentState_ != State_.READY) {
    speech.abort_(LogSource_.HOTWORD_VOICE_TRIGGER, LogReason_.NOT_READY);
  }
  speech.currentState_ = State_.STARTING;
  speech.start_();
};
*/


/**
 * Toggles starting and stopping of speech recognition by the speech tool.
 * @private
 */
sfs.speech.toggleStartStop_ = function() {
  if (speech.currentState_ == State_.DISABLED) {
    speech.logGen204_(LogSource_.DISABLED);
    return;
  }
  if (speech.currentState_ != State_.READY) {
    speech.abort_(LogSource_.CONTROLLER, LogReason_.NOT_READY);
  } else {
    // Chrome hotword opt-in UI can intercept a click on the microphone icon
    // to show a message instead of triggering a speech input.
    /*
    if (speech.hotwordUi_) {
      speech.hotwordUi_.maybeShowOptInMessage(function() {
        speech.currentState_ = State_.STARTING;
        speech.start_();
      });
    } else {
    */
    speech.currentState_ = State_.STARTING;
    speech.start_();
    // }
  }
};


/**
 * Handles click events during speech recognition.
 * @param {string} target The UI object that was clicked.
 * @param {string} additionalData Any additional data to be logged.
 * @param {boolean} shouldSubmit True if a query should be submitted.
 * @param {boolean} shouldRetry True if the interface should be restarted.
 * @private
 */
sfs.speech.handleClickEvent_ = function(
    target, additionalData, shouldSubmit, shouldRetry) {
  if (speech.speechQuery_ && shouldSubmit) {
    speech.submitSpeechQuery_(SearchMethod_.CLICK);
  } else if (speech.currentState_ == State_.STOPPED && shouldRetry) {
    speech.logGen204_(LogSource_.RETRY_CLICK, target, additionalData);
    speech.reset_();
    speech.toggleStartStop_();
  } else {
    speech.abort_(LogSource_.CLICK_ABORT, target, additionalData);
  }
};

// TODO(oskopek): Do we need this?
/*
gws.moduleInterface.register('spch', {
  [gws.moduleInterface.EventType.INIT]: speech.init_,
  [gws.moduleInterface.EventType.DISPOSE]: function() {
    if (!speech.isUiHidden_()) {
      speech.abort_(LogSource_.CONTROLLER, LogReason_.UPDATE_LOCATION);
    }
  }
});
*/

/* TEXT VIEW */
/**
 * @fileoverview Provides the implementation for the Speech text view.
 */

sfs.speech.textView = {};


/* @const */ var textView = sfs.speech.textView;

/**
 * ID for the link shown in error output.
 * @const
 */
sfs.speech.textView.ERROR_LINK_ID = 'sfs-text-area';


/**
 * Class name for the speech recognition error text link.
 * @const @private
 */
sfs.speech.textView.ERROR_LINK_CLASS_ = 'sfs-text-area';


/**
 * Class name for the speech recognition result output area.
 * @const @private
 */
sfs.speech.textView.TEXT_AREA_CLASS_ = 'sfs-text';


/**
 * ID of the final / high confidence speech recognition results element.
 * @const @private
 */
sfs.speech.textView.FINAL_TEXT_AREA_ID_ = 'sfs-text-f';


/**
 * ID of the interim / low confidence speech recognition results element.
 * @const @private
 */
sfs.speech.textView.INTERIM_TEXT_AREA_ID_ = 'sfs-text-i';


/**
 * Indicates that the text content contains HTML link tags.
 * @const @private
 */
sfs.speech.textView.HAS_LINK_ = true;


/**
 * The line height of the speech recognition results text.
 * @const @private
 */
sfs.speech.textView.LINE_HEIGHT_ = 1.2;


/**
 * Font size in the full page view in pixels.
 * @const @private
 */
sfs.speech.textView.FULL_PAGE_FONT_SIZE_ = 32;


/**
 * Delay in milliseconds before showing the initializing message.
 * @const @private
 */
sfs.speech.textView.INITIALIZING_TIMEOUT_MS_ = 300;


/**
 * Delay in milliseconds before showing the listening message.
 * @const @private
 */
sfs.speech.textView.LISTENING_TIMEOUT_MS_ = 2000;


/**
 * Delay in milliseconds between animation updates of the listening message.
 * @const @private
 */
sfs.speech.textView.LISTENING_ANIMATION_DELAY_MS_ = 30;


/**
 * The final / high confidence speech recognition result element.
 * @private {Element}
 */
sfs.speech.textView.final_;


/**
 * The interim / low confidence speech recognition result element.
 * @private {Element}
 */
sfs.speech.textView.interim_;


/**
 * Stores the size of the font for the current display.
 * @private {number}
 */
sfs.speech.textView.fontSize_;


/**
 * Stores the ID of the initializing message timer.
 * @private {number}
 */
sfs.speech.textView.initializingTimer_;


/**
 * Stores the ID of the listening message timer.
 * @private {number}
 */
sfs.speech.textView.listeningTimer_;


/**
 * Finds the text view elements.
 * @return {boolean} True if the text view was initialized properly.
 */
sfs.speech.textView.init = function() {
  textView.final_ = $(textView.FINAL_TEXT_AREA_ID_);
  textView.interim_ = $(textView.INTERIM_TEXT_AREA_ID_);
  textView.clear();

  return !!(textView.final_ && textView.interim_);
};


/**
 * Updates the text elements with new recognition results.
 * @param {string} interimText Low confidence speech recognition results.
 * @param {string} finalText High confidence speech recognition results.
 * @param {boolean=} opt_hasLink True if the string contains an HTML link.
 */
sfs.speech.textView.updateText = function(interimText, finalText, opt_hasLink) {
  window.clearTimeout(textView.initializingTimer_);
  textView.cancelListeningTimeout();

  textView.interim_.innerHTML = interimText;
  textView.final_.innerHTML = finalText;
  /* @const */ var link = textView.interim_.firstElementChild;

  // Is supposed to have link and (probably) contains an <a> element
  if (opt_hasLink && !!link) {
    link.id = textView.ERROR_LINK_ID;
    classlistSet(link, textView.ERROR_LINK_CLASS_);
  }

  textView.interim_.className = textView.final_.className =
      textView.getTextClassName_();
};


/**
 * Sets the text view to the initializing state.
 */
sfs.speech.textView.showInitializingMessage = function() {
  /* @const */ var displayMessage = function() {
    if (textView.interim_.innerText == '') {
      textView.updateText(sfs.speech.messages.waiting, '');
    }
  };

  textView.interim_.innerText = '';
  textView.final_.innerText = '';

  // We give the interface some time to get the permission. Once permission
  // is obtained, the ready message is displayed, in which case the
  // initializing message won't be shown.
  textView.initializingTimer_ =
      window.setTimeout(displayMessage, textView.INITIALIZING_TIMEOUT_MS_);
};


/**
 * Sets the text view to the ready state.
 */
sfs.speech.textView.showReadyMessage = function() {
  window.clearTimeout(textView.initializingTimer_);
  textView.updateText(sfs.speech.messages.ready, '');
  textView.startListeningMessageAnimation_();
};


/**
 * Displays an error message.
 * @param {gws.speech.RecognitionError} error The error that occured.
 */
sfs.speech.textView.showErrorMessage = function(error) {
  switch (error) {
    case RecognitionError.NO_MATCH:
      textView.updateText(
          sfs.speech.messages.noTranslation, '', textView.HAS_LINK_);
      break;
    case RecognitionError.NO_SPEECH:
      textView.updateText(sfs.speech.messages.noVoice, '', textView.HAS_LINK_);
      break;
    case RecognitionError.AUDIO_CAPTURE:
      textView.updateText(
          sfs.speech.messages.audioError, '', textView.HAS_LINK_);
      break;
    case RecognitionError.NETWORK:
      textView.updateText(sfs.speech.messages.networkError, '');
      break;
    case RecognitionError.NOT_ALLOWED:
    case RecognitionError.SERVICE_NOT_ALLOWED:
      textView.updateText(
          sfs.speech.messages.permissionError, '', textView.HAS_LINK_);
      break;
    case RecognitionError.LANGUAGE_NOT_SUPPORTED:
      textView.updateText(sfs.speech.messages.languageError, '');
      break;
  }
};


/**
 * Clears the text elements.
 */
sfs.speech.textView.clear = function() {
  textView.cancelListeningTimeout();
  window.clearTimeout(textView.initializingTimer_);

  textView.fontSize_ = 0;

  classlistSet(textView.interim_, textView.TEXT_AREA_CLASS_);
  classlistSet(textView.final_, textView.TEXT_AREA_CLASS_);
};


/**
 * Cancels listening message display.
 */
sfs.speech.textView.cancelListeningTimeout = function() {
  window.clearTimeout(textView.listeningTimer_);
};


/**
 * Determines the class name of the text output Elements.
 * @return {string} The class name.
 * @private
 */
sfs.speech.textView.getTextClassName_ = function() {
  textView.updateFontSize_();

  // Shift up for every line.
  /* @const */ var oneLineHeight =
      textView.LINE_HEIGHT_ * textView.fontSize_ + 1;
  /* @const */ var twoLineHeight =
      textView.LINE_HEIGHT_ * textView.fontSize_ * 2 + 1;
  /* @const */ var threeLineHeight =
      textView.LINE_HEIGHT_ * textView.fontSize_ * 3 + 1;
  /* @const */ var fourLineHeight =
      textView.LINE_HEIGHT_ * textView.fontSize_ * 4 + 1;

  /* @const */ var height = textView.interim_.scrollHeight;
  let className = textView.TEXT_AREA_CLASS_;

  if (height > fourLineHeight) {
    className += ' spch-5l';
  } else if (height > threeLineHeight) {
    className += ' spch-4l';
  } else if (height > twoLineHeight) {
    className += ' spch-3l';
  } else if (height > oneLineHeight) {
    className += ' spch-2l';
  }
  return className;
};


/**
 * Updates the font size based on whether the current page is the homepage or
 * the search results page. This is only performed once per voice search as
 * the page may change due to psychic updates.
 * @private
 */
sfs.speech.textView.updateFontSize_ = function() {
  if (textView.fontSize_ != 0) {
    return;
  }
  textView.fontSize_ = textView.FULL_PAGE_FONT_SIZE_;
};


/**
 * Displays the listening message animation after the ready message has been
 * shown for textView.LISTENING_TIMEOUT_MS_ milliseconds without further user
 * action.
 * @private
 */
sfs.speech.textView.startListeningMessageAnimation_ = function() {
  let i = 0;
  let listeningText = '';

  /* @const */ var animateListeningText = function() {
    /* @const */ var listening = sfs.speech.messages.listening;
    /* @const */ var isAnimationDone = i == listening.length;
    /* @const */ var isAnimationInterrupted =
        i > 0 && textView.interim_.innerText != listening.substring(0, i);
    /* @const */ var shouldNotStartAnimation =
        i == 0 && textView.interim_.innerText != sfs.speech.messages.ready;

    if (isAnimationDone || isAnimationInterrupted || shouldNotStartAnimation) {
      return;
    }

    listeningText += listening.substring(i, i + 1);
    textView.updateText(listeningText, '');
    ++i;
    textView.listeningTimer_ = window.setTimeout(
        animateListeningText, textView.LISTENING_ANIMATION_DELAY_MS_);
  };

  textView.listeningTimer_ =
      window.setTimeout(animateListeningText, textView.LISTENING_TIMEOUT_MS_);
};
/* END TEXT VIEW */

/* MICROPHONE VIEW */
/**
 * @fileoverview Provides the implementation for the Speech microphone view.
 */

sfs.speech.microphoneView = {};

/* @const */ var microphoneView = sfs.speech.microphoneView;


/**
 * ID for the button Element.
 * @const
 */
sfs.speech.microphoneView.RED_BUTTON_ID = 'sfs-button';


/**
 * ID for the level animations Element that indicates input volume.
 * @const @private
 */
sfs.speech.microphoneView.LEVEL_ID_ = 'sfs-level';


/**
 * ID for the container of the microphone, red button and level animations.
 * @const @private
 */
sfs.speech.microphoneView.CONTAINER_ID_ = 'sfs-button-container';


/**
 * The transform scale applied to active elements.
 * @const @private
 */
sfs.speech.microphoneView.ACTIVE_TRANSFORM_SCALE_ = 'scale(1)';


/**
 * The transform scale applied to inactive elements.
 * @const @private
 */
sfs.speech.microphoneView.INACTIVE_TRANSFORM_SCALE_ = 'scale(.1)';


/**
 * The value of 1 second in milliseconds.
 * @const @private
 */
sfs.speech.microphoneView.SECOND_IN_MILLISECONDS_ = 1000;


/**
 * The minimum transform scale for the volume rings.
 * @const @private
 */
sfs.speech.microphoneView.LEVEL_SCALE_MINIMUM_ = 0.5;


/**
 * The range of the transform scale for the volume rings.
 * @const @private
 */
sfs.speech.microphoneView.LEVEL_SCALE_RANGE_ = 0.55;


/**
 * The minimum transition time (in milliseconds) for the volume rings.
 * @const @private
 */
sfs.speech.microphoneView.LEVEL_TIME_STEP_MINIMUM_ = 170;


/**
 * The range of the transition time for the volume rings.
 * @const @private
 */
sfs.speech.microphoneView.LEVEL_TIME_STEP_RANGE_ = 10;


/**
 * The button with the microphone icon.
 * @private {Element}
 */
sfs.speech.microphoneView.button_;


/**
 * The voice level element that is displayed when the user starts speaking.
 * @private {Element}
 */
sfs.speech.microphoneView.level_;


/**
 * Variable to indicate whether level animations are underway.
 * @private {boolean}
 */
sfs.speech.microphoneView.isLevelAnimating_ = false;


/**
 * Creates/finds the output elements for the microphone rendering and animation.
 * @return {boolean} True if the microphone view initialized properly.
 */
sfs.speech.microphoneView.init = function() {

  // Get the button element and microphone container.
  microphoneView.button_ = $(microphoneView.RED_BUTTON_ID);

  // Get the animation elements.
  microphoneView.level_ = $(microphoneView.LEVEL_ID_);

  // Make sure we got all of our required elements.
  return !!(microphoneView.button_ && microphoneView.level_);
};


/**
 * Starts the volume circles animations.
 */
sfs.speech.microphoneView.startInputAnimation = function() {
  if (!microphoneView.isLevelAnimating_) {
    microphoneView.isLevelAnimating_ = true;
    microphoneView.runLevelAnimation_();
  }
};


/**
 * Stops the volume circles animations.
 */
sfs.speech.microphoneView.stopInputAnimation = function() {
  microphoneView.isLevelAnimating_ = false;
};


/**
 * Runs the volume level animation.
 * @private
 */
sfs.speech.microphoneView.runLevelAnimation_ = function() {
  // TODO(aayushkumar): Figure out a good way to get the css prefix here.
  /* @const */ var transitionText = '-webkit-transition';
  /* @const */ var transformText = '-webkit-transform';
  if (!microphoneView.isLevelAnimating_) {
    microphoneView.level_.style.removeProperty('opacity');
    microphoneView.level_.style.removeProperty(transitionText);
    microphoneView.level_.style.removeProperty(transformText);
    return;
  }
  /* @const */ var scale = microphoneView.LEVEL_SCALE_MINIMUM_ +
      Math.random() * microphoneView.LEVEL_SCALE_RANGE_;
  /* @const */ var timeStep = Math.round(
      microphoneView.LEVEL_TIME_STEP_MINIMUM_ +
      Math.random() * microphoneView.LEVEL_TIME_STEP_RANGE_);
  /* @const */ var transitionInterval =
      timeStep / microphoneView.SECOND_IN_MILLISECONDS_;
  microphoneView.level_.style.setProperty(
      transitionText,
      transformText + ' ' + transitionInterval + 's ease-in-out');
  microphoneView.level_.style.setProperty(
      transformText, 'scale(' + scale + ')');
  window.setTimeout(microphoneView.runLevelAnimation_, timeStep);
};

/* END MICROPHONE VIEW */

/* VIEW */

/**
 * The set of user aborted states.
 * @enum {string}
 * @private
 */
sfs.view.ClickTarget_ = {
  X: '0',
  RED_BUTTON: '1',
  BACKGROUND: '2',
  SPEECH_TEXT_AREA: '3',
  UNKNOWN_AREA: '4',
  ERROR_LINK: '5'
};

/**
 * Class name of the speech recognition interface on the homepage.
 * @const @private
 */
sfs.view.FULL_PAGE_CLASS_ = 'overlay sfs';


/**
 * Class name of the speech recognition interface when it is hidden on the
 * homepage.
 * @const @private
 */
sfs.view.FULL_PAGE_HIDDEN_CLASS_ = 'overlay overlay-hidden';


/**
 * ID for the speech output background.
 * @const @private
 */
sfs.view.BACKGROUND_ID_ = 'sfs-overlay';


/**
 * Background color used in the top bar view to partially show the search
 * results.
 * @const @private
 */
sfs.view.TRANSLUCENT_BACKGROUND_COLOR_ = 'rgba(255, 255, 255, 0.9)';


/**
 * ID of the close (x) button.
 * @const @private
 */
sfs.view.CLOSE_BUTTON_ID_ = 'sfs-close-button';


/**
 * ID for the speech output container.
 * @const @private
 */
sfs.view.CONTAINER_ID_ = 'sfs-outer';


/**
 * Class name used to modify the UI to the 'waiting for permission' state.
 * @const @private
 */
sfs.view.WAITING_FOR_PERMISSION_CLASS_ = 'outer sfs-fp';


/**
 * Class name used to modify the UI to the 'listening' state.
 * @const @private
 */
sfs.view.MICROPHONE_LISTENING_CLASS_ = 'outer sfs-ml';


/**
 * Class name used to modify the UI to the 'receiving audio' state.
 * @const @private
 */
sfs.view.RECEIVING_AUDIO_CLASS_ = 'outer sfs-ra';


/**
 * Class name used to modify the UI to the 'error received' state.
 * @const @private
 */
sfs.view.ERROR_RECEIVED_CLASS_ = 'outer sfs-er';


/**
 * Class name used to modify the UI to the inactive state.
 * @const @private
 */
sfs.view.INACTIVE_CLASS_ = 'outer';


/**
 * Background element and container of all other elements.
 * @private {Element}
 */
sfs.view.background_;


/**
 * The container used to position the microphone and text output area.
 * @private {Element}
 */
sfs.view.container_;


/**
 * True if the the last error message shown was for the 'no-match' error.
 * @private {boolean}
 */
sfs.view.isNoMatchShown_ = false;


/**
 * True if the UI elements are visible.
 * @private {boolean}
 */
sfs.view.isVisible_ = false;


/**
 * The function to call when there is a click event.
 * @private {!view.ClickHandler}
 */
sfs.view.onClick_;


/**
 * Displays the UI.
 */
sfs.view.show = function() {
  if (!view.isVisible_) {
    textView.showInitializingMessage();
    view.showView_();
    window.addEventListener(
        sfs.EventType.MOUSEUP, view.handleWindowClick_, false);
  }
};

function classlistSet(element, className) {
  element.className = className;
}

/**
 * Sets the output area text to listening. This should only be called when
 * the Web Speech API is receiving audio input (i.e., onaudiostart).
 */
sfs.view.setReadyForSpeech = function() {
  if (view.isVisible_) {
    classlistSet(view.container_, view.MICROPHONE_LISTENING_CLASS_);
    textView.showReadyMessage();
  }
};


/**
 * Shows the pulsing animation emanating from the microphone. This should only
 * be called when the Web Speech API is receiving speech input (i.e.,
 * onspeechstart).
 */
sfs.view.setReceivingSpeech = function() {
  if (view.isVisible_) {
    classlistSet(view.container_, view.RECEIVING_AUDIO_CLASS_);
    microphoneView.startInputAnimation();
    textView.cancelListeningTimeout();
  }
};


/**
 * Updates the speech recognition results output with the latest results.
 * @param {string} interimResultText Low confidence recognition text (grey).
 * @param {string} finalResultText High confidence recognition text (black).
 */
sfs.view.updateSpeechResult = function(interimResultText, finalResultText) {
  if (view.isVisible_) {
    classlistSet(view.container_, view.RECEIVING_AUDIO_CLASS_);
    textView.updateText(interimResultText, finalResultText);
  }
};


/**
 * Shows the permission bar decorator.
 */
sfs.view.setWaitingForPermission = function() {
  if (view.isVisible_) {
    classlistSet(view.container_, view.WAITING_FOR_PERMISSION_CLASS_);
  }
};


/**
 * Hides the UI and stops animations.
 */
sfs.view.hide = function() {
  window.removeEventListener(
      sfs.EventType.MOUSEUP, view.handleWindowClick_, false);
  view.stopMicrophoneAnimations_();
  view.hideView_();
  view.isNoMatchShown_ = false;
  textView.clear();
};


/**
 * Find the page elements that will be used to render the speech recognition
 * interface area.
 * @param {!view.ClickHandler} onClick The function to call when
 *     there is a click event in the window.
 * @return {boolean} True if the view and it's subviews initialized properly.
 */
sfs.view.init = function(onClick) {
  view.background_ = $(view.BACKGROUND_ID_);
  view.container_ = $(view.CONTAINER_ID_);
  view.onClick_ = onClick;

  return !!view.background_ && !!view.container_ && !!textView.init() &&
      microphoneView.init();
};


/**
 * Displays an error message and stops animations.
 * @param {RecognitionError} error The error type.
 */
sfs.view.showError = function(error) {
  classlistSet(view.container_, view.ERROR_RECEIVED_CLASS_);
  textView.showErrorMessage(error);
  view.stopMicrophoneAnimations_();
  if (error == RecognitionError.NO_MATCH) {
    view.isNoMatchShown_ = true;
  }
};


/**
 * Makes the view visible.
 * @private
 */
sfs.view.showView_ = function() {
  if (!view.isVisible_) {
    view.background_.hidden = false;
    view.showFullPage_();
    view.isVisible_ = true;
  }
};


/**
 * Displays the full page view, animating from the hidden state to the visible
 * state.
 * @private
 */
sfs.view.showFullPage_ = function() {
  classlistSet(view.background_, view.FULL_PAGE_HIDDEN_CLASS_);
  classlistSet(view.background_, view.FULL_PAGE_CLASS_);
};


/**
 * Hides the view.
 * @private
 */
sfs.view.hideView_ = function() {
  classlistSet(view.background_, view.FULL_PAGE_HIDDEN_CLASS_);
  classlistSet(view.container_, view.INACTIVE_CLASS_);
  view.background_.removeAttribute('style');
  view.background_.hidden = true;
  view.isVisible_ = false;
};


/**
 * Stops the animations in the microphone view.
 * @private
 */
sfs.view.stopMicrophoneAnimations_ = function() {
  microphoneView.stopInputAnimation();
};


/**
 * Makes sure that a click anywhere closes the ui when it is active.
 * @param {Event} event The click event.
 * @private
 */
sfs.view.handleWindowClick_ = function(event) {
  if (!view.isVisible_) {
    return;
  }
  /* @const */
  var targetId = event.target.id;
  var clickTarget = sfs.view.ClickTarget_.UNKNOWN_AREA;
  var additionalData = '';
  if (targetId == view.CLOSE_BUTTON_ID_) {
    clickTarget = sfs.view.ClickTarget_.X;
  } else if (targetId == view.BACKGROUND_ID_) {
    clickTarget = sfs.view.ClickTarget_.BACKGROUND;
  } else if (targetId == microphoneView.RED_BUTTON_ID) {
    clickTarget = sfs.view.ClickTarget_.RED_BUTTON;
  } else if (targetId == textView.ERROR_LINK_ID) {
    clickTarget = sfs.view.ClickTarget_.ERROR_LINK;
  } else {
    additionalData = targetId;
  }

  /* @const */
  var shouldRetry = (clickTarget == sfs.view.ClickTarget_.RED_BUTTON ||
                     clickTarget == sfs.view.ClickTarget_.ERROR_LINK) &&
      view.isNoMatchShown_;

  /* @const */
  var submitQuery =
      clickTarget == sfs.view.ClickTarget_.RED_BUTTON && !view.isNoMatchShown_;

  view.onClick_(clickTarget, additionalData, submitQuery, shouldRetry);
};


/* END VIEW */
