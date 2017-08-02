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


/**
 * General pseudo-module for Voice Search.
 */
var voice = {};


/**
 * Provides methods for communicating with the <a
 * href="https://developer.mozilla.org/en-US/docs/Web/API/Web_Speech_API">
 * Web Speech API</a>, error handling and executing search queries.
 */
voice.speech = {};


/**
 * Provides methods for manipulating and animating the Voice Search
 * full screen overlay.
 */
voice.view = {};


/**
 * Provides methods for animating the microphone button and icon.
 */
voice.view.microphone = {};


/**
 * Provides methods for styling and animating the text areas
 * left of the microphone button.
 */
voice.view.text = {};


/**
 * Enum for keycodes.
 * @enum {number}
 * @const
 */
var KEYCODE = {ENTER: 13, ESC: 27, PERIOD: 190};


/**
 * The set of possible recognition errors.
 * @enum {string}
 */
voice.speech.RecognitionError = {
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
voice.speech.messages = {
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


/**
 * The set of reasons the controller generates logs.
 * @enum {string}
 * @private
 */
voice.speech.LogReason_ = {
  ESCAPE_KEY: '0',
  WEBKIT_SPEECH_RECOGNITION: '1',
  UPDATE_LOCATION: '2',
  IDLE_TIMEOUT: '3',
  VISIBILITY_CHANGE: '4',
  NOT_READY: '5',
  FOCUS_CHANGE: '6'
};


/**
 * The sources of logs to indicate what the logs were issued for.
 * @enum {number}
 * @private
 */
voice.speech.LogSource_ = {
  CONTROLLER: 0,
  CLICK_ABORT: 1,
  API_ERROR: 2,
  CONTROLLER_ERROR: 3,
  NO_MATCH: 4,
  RETRY_CLICK: 6
};


/**
 * The set of controller states.
 * @enum {number}
 * @private
 */
voice.speech.State_ = {
  UNINITIALIZED: -1,
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


// Shortcuts for pseudo-modules and enums:
/* @const */ var speech = voice.speech;
/* @const */ var view = voice.view;
/* @const */ var microphone = voice.view.microphone;
/* @const */ var text = voice.view.text;
/* @const */ var RecognitionError = voice.speech.RecognitionError;
/* @const */ var LogReason_ = voice.speech.LogReason_;
/* @const */ var LogSource_ = voice.speech.LogSource_;
/* @const */ var State_ = voice.speech.State_;


/**
 * Time in milliseconds to wait before closing the UI after an error has
 * occured. This is a short timeout used when no click-target is present.
 * @private {number}
 * @const
 */
voice.speech.ERROR_TIMEOUT_SHORT_MS_ = 3000;


/**
 * Time in milliseconds to wait before closing the UI after an error has
 * occured. This is a longer timeout used when there is a click-target is
 * present.
 * @private {number}
 * @const
 */
voice.speech.ERROR_TIMEOUT_LONG_MS_ = 8000;


/**
 * Time in milliseconds to wait before closing the UI if no interaction has
 * occured.
 * @private {number}
 * @const
 */
voice.speech.IDLE_TIMEOUT_MS_ = 8000;


/**
 * Specifies the current state of the controller.
 * @private {State_}
 */
voice.speech.currentState_ = State_.UNINITIALIZED;


/**
 * The ID for the error timer.
 * @private {number}
 */
voice.speech.errorTimer_;


/**
 * The duration of the timeout for the UI elements during an error state.
 * Depending on the error state, we have different durations for the timeout.
 * @private {number}
 */
voice.speech.errorTimeoutMs_ = 0;


/**
 * The last high confidence voice transcript received.
 * @private {string}
 */
voice.speech.finalResult_;


/**
 * The ID for the idle timer.
 * @private {number}
 */
voice.speech.idleTimer_;


/**
 * The last low confidence voice transcript received.
 * @private {string}
 */
voice.speech.interimResult_;


/**
 * The Web Speech API element driving the whole process.
 * @private {!webkitSpeechRecognition}
 */
voice.speech.recognition_;


/**
 * The query used to fetch the last set of speech results.
 * @private {string}
 */
voice.speech.speechQuery_;


/**
 * Initializes the speech module.
 * @param {!Object} configData The NTP configuration.
 * @private
 */
voice.speech.init_ = function(configData) {
  if (speech.currentState_ != State_.UNINITIALIZED) {
    throw new Error(
        'Trying to re-initialize speech when not in UNINITIALIZED state.');
  }

  view.init(speech.handleClickEvent_);
  // Set translations map.
  speech.messages = {
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
};


/**
 * Initializes the voice search element.
 * @private
 */
voice.speech.initWebkitSpeech_ = function() {
  speech.recognition_ = new webkitSpeechRecognition();
  speech.recognition_.continuous = false;
  speech.recognition_.interimResults = true;
  speech.recognition_.lang = window.navigator.language;
  speech.recognition_.maxAlternatives = 4;
  speech.recognition_.onerror = speech.handleRecognitionError_;
  // speech.recognition_.onnomatch = speech.handleRecognitionOnNoMatch_;
  // speech.recognition_.onend = speech.handleRecognitionEnd_;
  // speech.recognition_.onresult = speech.handleRecognitionResult_;
  speech.recognition_.onaudiostart = speech.handleRecognitionAudioStart_;
  speech.recognition_.onspeechstart = speech.handleRecognitionSpeechStart_;
};


/**
 * Sets up the necessary states for voice search and then starts the
 * speech recognition interface.
 * @private
 */
voice.speech.start_ = function() {
  if (speech.currentState_ == State_.STARTING) {
    view.show();

    speech.resetIdleTimer_(speech.IDLE_TIMEOUT_MS_);

    document.addEventListener(
        'webkitvisibilitychange', speech.handleVisibilityChange_, false);

    if (!speech.isRecognitionInitialized_()) {
      speech.initWebkitSpeech_();
    }

    // TODO(oskopek): Used to fail when start_() was called too quickly after
    // stop_(). Verify that it does not anymore. Add tests.
    try {
      speech.recognition_.start();
      speech.currentState_ = State_.STARTED;
    } catch (error) {
      speech.currentState_ = State_.ABORTED;
      speech.log_(LogSource_.CONTROLLER, LogReason_.WEBKIT_SPEECH_RECOGNITION);
      speech.stop_();
    }
  }
};


/**
 * Stops the speech recognition interface and restores the input field value.
 * @private
 */
voice.speech.stop_ = function() {
  if (speech.currentState_ == State_.ABORTED) {
    speech.currentState_ = State_.STOPPED;
    speech.recognition_.abort();
  }
  view.hide();
  speech.reset_();
};


/**
 * Aborts the speech recognition interface and logs the abortion.
 * @param {LogSource_} abortedBy The source of the abort.
 * @param {!LogReason_|string} reason The reason for the abort.
 * @param {string=} opt_additionalData Any additional data to be logged.
 * @private
 */
voice.speech.abort_ = function(abortedBy, reason, opt_additionalData) {
  speech.log_(abortedBy, reason, opt_additionalData);
  speech.currentState_ = State_.ABORTED;
  speech.stop_();
};


/**
 * Resets the speech recognition interface.
 * @private
 */
voice.speech.reset_ = function() {
  window.clearTimeout(speech.idleTimer_);
  window.clearTimeout(speech.errorTimer_);

  document.removeEventListener(
      'webkitvisibilitychange', speech.handleVisibilityChange_, false);

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
voice.speech.handleRecognitionAudioStart_ = function(opt_event) {
  speech.resetIdleTimer_(speech.IDLE_TIMEOUT_MS_);
  speech.currentState_ = State_.AUDIO_RECEIVED;
  view.setReadyForSpeech();
};


/**
 * Function is called when the user starts speaking.
 * @param {Event=} opt_event Emitted event for speech start.
 * @private
 */
voice.speech.handleRecognitionSpeechStart_ = function(opt_event) {
  speech.resetIdleTimer_(speech.IDLE_TIMEOUT_MS_);
  speech.currentState_ = State_.SPEECH_RECEIVED;
  view.setReceivingSpeech();
};


/**
 * Called when an error from Web Speech API is received.
 * @param {SpeechRecognitionError} error The error event.
 * @private
 */
voice.speech.handleRecognitionError_ = function(error) {
  speech.resetIdleTimer_(speech.IDLE_TIMEOUT_MS_);
  /* @const */ var speechError = speech.getRecognitionError_(error.error);
  speech.errorTimeoutMs_ = speech.getRecognitionErrorTimeout_(speechError);
  if (speechError != RecognitionError.ABORTED) {
    let additionalData = '';
    if (speechError == RecognitionError.OTHER) {
      additionalData = error.error;
    }
    speech.log_(LogSource_.API_ERROR, speechError, additionalData);
    speech.currentState_ = State_.ERROR_RECEIVED;
    view.showError(speechError);
    window.clearTimeout(speech.idleTimer_);
    speech.resetErrorTimer_(speech.errorTimeoutMs_);
  }
};


/**
 * Handles the following keyboard actions.
 * - <CTRL> + <SHIFT> + <.> starts voice input(<CMD> + <SHIFT> + <.> on mac).
 * - <ESC> aborts voice input when the recognition interface is active.
 * - <ENTER> submits the speech query if there is one.
 * @param {KeyboardEvent} event The keydown event.
 * @private
 */
voice.speech.handleKeyDown_ = function(event) {
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
  } else {
    // Ensures that keyboard events are not propagated to the Searchbox client
    // during voice input.
    event.stopPropagation();
    if (event.keyCode == KEYCODE.ESC) {
      speech.abort_(LogSource_.CONTROLLER, LogReason_.ESCAPE_KEY);
    } else if (event.keyCode == KEYCODE.ENTER && speech.speechQuery_) {
      speech.submitSpeechQuery_();
    }
  }
};


/**
 * Stops the recognition interface and closes the UI if no interactions occur
 * after some time and the interface is still active. This is a safety net in
 * case the recognition.onend event doesn't fire, as is sometime the case. If
 * a high confidence transcription was received then show the search results.
 * @private
 */
voice.speech.handleIdleTimeout_ = function() {
  if (speech.finalResult_ != '') {
    speech.speechQuery_ = speech.finalResult_;
    speech.submitSpeechQuery_();
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
 * Aborts the speech recognition interface when the user switches to a new
 * tab or window.
 * @private
 */
voice.speech.handleVisibilityChange_ = function() {
  if (speech.isUiHidden_()) {
    return;
  }

  if (document.webkitHidden) {
    speech.abort_(LogSource_.CONTROLLER, LogReason_.VISIBILITY_CHANGE);
  }
};


/**
 * Aborts the speech session if the UI is showing and omnibox gets focused.
 * @private
 */
voice.speech.handleOmniboxFocused_ = function() {
  if (!speech.isUiHidden_()) {
    speech.abort_(LogSource_.CONTROLLER, LogReason_.FOCUS_CHANGE);
  }
};


/**
 * Submits the spoken speech query to perform a search on.
 * @private
 */
voice.speech.submitSpeechQuery_ = function() {
  speech.currentState_ = State_.SEARCH_SUBMITTED;
  window.clearTimeout(speech.idleTimer_);

  // TODO(oskopek): Send query.
  speech.stop_();
};


/**
 * Returns the error type based on the error string received from the webkit
 * speech recognition API.
 * @param {string} error The error string received from the webkit speech
 *     recognition API.
 * @return {RecognitionError} The appropriate error state from
 *     the RecognitionError enum.
 * @private
 */
voice.speech.getRecognitionError_ = function(error) {
  switch (error) {
    case 'aborted':
      return RecognitionError.ABORTED;
    case 'audio-capture':
      return RecognitionError.AUDIO_CAPTURE;
    case 'bad-grammar':
      return RecognitionError.BAD_GRAMMAR;
    case 'language-not-supported':
      return RecognitionError.LANGUAGE_NOT_SUPPORTED;
    case 'network':
      return RecognitionError.NETWORK;
    case 'no-speech':
      return RecognitionError.NO_SPEECH;
    case 'not-allowed':
      return RecognitionError.NOT_ALLOWED;
    case 'service-not-allowed':
      return RecognitionError.SERVICE_NOT_ALLOWED;
    default:
      return RecognitionError.OTHER;
  }
};

/**
 * Returns a timeout based on the error received from the webkit speech
 * recognition API.
 * @param {RecognitionError} error The error received from the webkit speech
 *     recognition API.
 * @return {int} The appropriate error state from the RecognitionError enum.
 * @private
 */
voice.speech.getRecognitionErrorTimeout_ = function(error) {
  switch (error) {
    case RecognitionError.AUDIO_CAPTURE:
    case RecognitionError.NO_SPEECH:
    case RecognitionError.NOT_ALLOWED:
    case RecognitionError.SERVICE_NOT_ALLOWED:
      return speech.ERROR_TIMEOUT_LONG_MS_;
    default:
      return speech.ERROR_TIMEOUT_SHORT_MS_;
  }
};


/**
 * Log utility method.
 * @param {string=} opt_reason The reason for which the source generated the
 *     log.
 * @param {string=} opt_additionalData Any additional data that needs to be
 *     logged.
 * @private
 */
voice.speech.log_ = function(cad, opt_reason, opt_additionalData) {
  // TODO(oskopek): Remove this method or enable logging only in dev builds.
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

  var logSource = findInverseString(speech.LogSource_, cad);
  var logReason = findInverseString(speech.LogReason_, opt_reason);
  console.log(
      `[VoiceSearch]: Source: ${logSource} (${opt_reason}/${logReason}).` +
      (opt_additionalData ? ` Data: ${opt_additionalData}` : ''));
};


/**
 * Resets the idle state timeout.
 * @param {number} duration The duration after which to close the UI.
 * @private
 */
voice.speech.resetIdleTimer_ = function(duration) {
  window.clearTimeout(speech.idleTimer_);
  speech.idleTimer_ = window.setTimeout(speech.handleIdleTimeout_, duration);
};


/**
 * Resets the idle error state timeout.
 * @param {number} duration The duration after which to close the UI during an
 *     error state.
 * @private
 */
voice.speech.resetErrorTimer_ = function(duration) {
  window.clearTimeout(speech.errorTimer_);
  speech.errorTimer_ = window.setTimeout(speech.stop_, duration);
};


/**
 * Check to see if the speech recognition interface is running.
 * @return {boolean} True, if the speech recognition interface is running.
 * @private
 */
voice.speech.isRecognizing_ = function() {
  switch (speech.currentState_) {
    case State_.READY:
    case State_.ABORTED:
    case State_.STOPPED:
    case State_.ERROR_RECEIVED:
    case State_.INITIALIZING:
      return false;
  }
  return true;
};


/**
 * Check if the controller is in a state where the UI is definitely hidden.
 * Since we show the UI for a few seconds after we receive an error from the
 * API, we need a separate definition to isRecognizing to indicate when the UI
 * is hidden.
 * @return {boolean} True if the UI is hidden.
 * @private
 */
voice.speech.isUiHidden_ = function() {
  switch (speech.currentState_) {
    case State_.READY:
    case State_.INITIALIZING:
      return true;
  }
  return false;
};


/**
 * Check if the Web Speech API is initialized and event functions are set.
 * @return {boolean} True if recognition is initialized.
 * @private
 */
voice.speech.isRecognitionInitialized_ = function() {
  // TODO(oskopek): Do handlers get reset by Chrome? Verify and test.
  return !!speech.recognition_;
};


/**
 * Toggles starting and stopping of speech recognition by the speech tool.
 * @private
 */
voice.speech.toggleStartStop_ = function() {
  if (speech.currentState_ != State_.READY) {
    speech.abort_(LogSource_.CONTROLLER, LogReason_.NOT_READY);
  } else {
    speech.currentState_ = State_.STARTING;
    speech.start_();
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
voice.speech.handleClickEvent_ = function(
    target, additionalData, shouldSubmit, shouldRetry) {
  if (speech.speechQuery_ && shouldSubmit) {
    speech.submitSpeechQuery_();
  } else if (speech.currentState_ == State_.STOPPED && shouldRetry) {
    speech.log_(LogSource_.RETRY_CLICK, target, additionalData);
    speech.restart_();
  } else {
    speech.abort_(LogSource_.CLICK_ABORT, target, additionalData);
  }
};

/**
 * Restarts voice recognition. Used for the 'Try again' error link.
 */
voice.speech.restart_ = function() {
  speech.reset_();
  speech.toggleStartStop_();
};


/* TEXT VIEW */
/**
 * ID for the link shown in error output.
 * @const
 */
voice.view.text.ERROR_LINK_ID = 'voice-text-area';


/**
 * Class name for the speech recognition error text link.
 * @const @private
 */
voice.view.text.ERROR_LINK_CLASS_ = 'voice-text-area';


/**
 * Class name for the speech recognition result output area.
 * @const @private
 */
voice.view.text.TEXT_AREA_CLASS_ = 'voice-text';

/**
 * Class name for the "Listening..." text animation.
 * @const @private
 */
voice.view.text.LISTENING_ANIMATION_CLASS_ = 'voice-text listening-animation';


/**
 * ID of the final / high confidence speech recognition results element.
 * @const @private
 */
voice.view.text.FINAL_TEXT_AREA_ID_ = 'voice-text-f';


/**
 * ID of the interim / low confidence speech recognition results element.
 * @const @private
 */
voice.view.text.INTERIM_TEXT_AREA_ID_ = 'voice-text-i';


/**
 * The line height of the speech recognition results text.
 * @const @private
 */
voice.view.text.LINE_HEIGHT_ = 1.2;


/**
 * Font size in the full page view in pixels.
 * @const @private
 */
voice.view.text.FONT_SIZE_ = 32;


/**
 * Delay in milliseconds before showing the initializing message.
 * @const @private
 */
voice.view.text.INITIALIZING_TIMEOUT_MS_ = 300;


/**
 * Delay in milliseconds before showing the listening message.
 * @const @private
 */
voice.view.text.LISTENING_TIMEOUT_MS_ = 2000;


/**
 * The final / high confidence speech recognition result element.
 * @private {Element}
 */
voice.view.text.final_;


/**
 * The interim / low confidence speech recognition result element.
 * @private {Element}
 */
voice.view.text.interim_;


/**
 * Stores the ID of the initializing message timer.
 * @private {number}
 */
voice.view.text.initializingTimer_;


/**
 * Stores the ID of the listening message timer.
 * @private {number}
 */
voice.view.text.listeningTimer_;


/**
 * Finds the text view elements.
 */
voice.view.text.init = function() {
  // TODO(oskopek): Test for the presenece of these elements.
  text.final_ = $(text.FINAL_TEXT_AREA_ID_);
  text.interim_ = $(text.INTERIM_TEXT_AREA_ID_);
  text.clear();
};


/**
 * Updates the text elements with new recognition results.
 * @param {string} interimText Low confidence speech recognition results.
 * @param {string} opt_finalText High confidence speech recognition results,
 *    defaults to an empty string.
 */
voice.view.text.updateText = function(interimText, opt_finalText) {
  var finalText = !!opt_finalText ? opt_finalText : '';

  window.clearTimeout(text.initializingTimer_);
  text.cancelListeningTimeout();

  text.interim_.innerHTML = interimText;
  text.final_.innerHTML = finalText;
  /* @const */ var link = text.interim_.firstElementChild;

  // If interim text contains an <a> element (used in error messages).
  if (!!link) {
    link.id = text.ERROR_LINK_ID;
    link.className = text.ERROR_LINK_CLASS_;
  }

  text.interim_.className = text.final_.className = text.getTextClassName_();
};


/**
 * Sets the text view to the initializing state.
 */
voice.view.text.showInitializingMessage = function() {
  /* @const */ var displayMessage = function() {
    if (text.interim_.innerText == '') {
      text.updateText(speech.messages.waiting);
    }
  };

  text.interim_.innerText = '';
  text.final_.innerText = '';

  // We give the interface some time to get the permission. Once permission
  // is obtained, the ready message is displayed, in which case the
  // initializing message won't be shown.
  text.initializingTimer_ =
      window.setTimeout(displayMessage, text.INITIALIZING_TIMEOUT_MS_);
};


/**
 * Sets the text view to the ready state.
 */
voice.view.text.showReadyMessage = function() {
  window.clearTimeout(text.initializingTimer_);
  text.updateText(speech.messages.ready);
  text.startListeningMessageAnimation_();
};


/**
 * Returns an error message based on the error.
 * @param {RecognitionError} error The error that occured.
 */
voice.view.text.getErrorMessage = function(error) {
  switch (error) {
    case RecognitionError.NO_MATCH:
      return speech.messages.noTranslation;
    case RecognitionError.NO_SPEECH:
      return speech.messages.noVoice;
    case RecognitionError.AUDIO_CAPTURE:
      return speech.messages.audioError;
    case RecognitionError.NETWORK:
      return speech.messages.networkError;
    case RecognitionError.NOT_ALLOWED:
    case RecognitionError.SERVICE_NOT_ALLOWED:
      return speech.messages.permissionError;
    case RecognitionError.LANGUAGE_NOT_SUPPORTED:
      return speech.messages.languageError;
    default:
      throw new Error('Illegal RecognitionError value: ' + error);
  }
};


/**
 * Clears the text elements.
 */
voice.view.text.clear = function() {
  text.cancelListeningTimeout();
  window.clearTimeout(text.initializingTimer_);

  text.interim_.className = text.TEXT_AREA_CLASS_;
  text.final_.className = text.TEXT_AREA_CLASS_;
};


/**
 * Cancels listening message display.
 */
voice.view.text.cancelListeningTimeout = function() {
  window.clearTimeout(text.listeningTimer_);
};


/**
 * Determines the class name of the text output Elements.
 * @return {string} The class name.
 * @private
 */
voice.view.text.getTextClassName_ = function() {
  // Shift up for every line.
  /* @const */ var oneLineHeight = text.LINE_HEIGHT_ * text.FONT_SIZE_ + 1;
  /* @const */ var twoLineHeight = text.LINE_HEIGHT_ * text.FONT_SIZE_ * 2 + 1;
  /* @const */ var threeLineHeight =
      text.LINE_HEIGHT_ * text.FONT_SIZE_ * 3 + 1;
  /* @const */ var fourLineHeight = text.LINE_HEIGHT_ * text.FONT_SIZE_ * 4 + 1;

  /* @const */ var height = text.interim_.scrollHeight;
  let className = text.TEXT_AREA_CLASS_;

  if (height > fourLineHeight) {
    className += ' voice-text-5l';
  } else if (height > threeLineHeight) {
    className += ' voice-text-4l';
  } else if (height > twoLineHeight) {
    className += ' voice-text-3l';
  } else if (height > oneLineHeight) {
    className += ' voice-text-2l';
  }
  return className;
};


/**
 * Displays the listening message animation after the ready message has been
 * shown for text.LISTENING_TIMEOUT_MS_ milliseconds without further user
 * action.
 * @private
 */
voice.view.text.startListeningMessageAnimation_ = function() {
  /* @const */ var animateListeningText = function() {
    if (text.interim_.innerText == speech.messages.ready) {
      text.updateText(speech.messages.listening);
      text.interim_.className = text.LISTENING_ANIMATION_CLASS_;
    }
  };

  text.listeningTimer_ =
      window.setTimeout(animateListeningText, text.LISTENING_TIMEOUT_MS_);
};
/* END TEXT VIEW */


/* MICROPHONE VIEW */
/**
 * ID for the button Element.
 * @const
 */
voice.view.microphone.RED_BUTTON_ID = 'voice-button';


/**
 * ID for the level animations Element that indicates input volume.
 * @const @private
 */
voice.view.microphone.LEVEL_ID_ = 'voice-level';


/**
 * ID for the container of the microphone, red button and level animations.
 * @const @private
 */
voice.view.microphone.CONTAINER_ID_ = 'voice-button-container';


/**
 * The transform scale applied to active elements.
 * @const @private
 */
voice.view.microphone.ACTIVE_TRANSFORM_SCALE_ = 'scale(1)';


/**
 * The transform scale applied to inactive elements.
 * @const @private
 */
voice.view.microphone.INACTIVE_TRANSFORM_SCALE_ = 'scale(.1)';


/**
 * The value of 1 second in milliseconds.
 * @const @private
 */
voice.view.microphone.SECOND_IN_MILLISECONDS_ = 1000;


/**
 * The minimum transform scale for the volume rings.
 * @const @private
 */
voice.view.microphone.LEVEL_SCALE_MINIMUM_ = 0.5;


/**
 * The range of the transform scale for the volume rings.
 * @const @private
 */
voice.view.microphone.LEVEL_SCALE_RANGE_ = 0.55;


/**
 * The minimum transition time (in milliseconds) for the volume rings.
 * @const @private
 */
voice.view.microphone.LEVEL_TIME_STEP_MINIMUM_ = 170;


/**
 * The range of the transition time for the volume rings.
 * @const @private
 */
voice.view.microphone.LEVEL_TIME_STEP_RANGE_ = 10;


/**
 * The button with the microphone icon.
 * @private {Element}
 */
voice.view.microphone.button_;


/**
 * The voice level element that is displayed when the user starts speaking.
 * @private {Element}
 */
voice.view.microphone.level_;


/**
 * Variable to indicate whether level animations are underway.
 * @private {boolean}
 */
voice.view.microphone.isLevelAnimating_ = false;


/**
 * Creates/finds the output elements for the microphone rendering and animation.
 */
voice.view.microphone.init = function() {
  // TODO(oskopek): Test for the presence of these elements.
  // Get the button element and microphone container.
  microphone.button_ = $(microphone.RED_BUTTON_ID);

  // Get the animation elements.
  microphone.level_ = $(microphone.LEVEL_ID_);
};


/**
 * Starts the volume circles animations.
 */
voice.view.microphone.startInputAnimation = function() {
  if (!microphone.isLevelAnimating_) {
    microphone.isLevelAnimating_ = true;
    microphone.runLevelAnimation_();
  }
};


/**
 * Stops the volume circles animations.
 */
voice.view.microphone.stopInputAnimation = function() {
  microphone.isLevelAnimating_ = false;
};


/**
 * Runs the volume level animation.
 * @private
 */
voice.view.microphone.runLevelAnimation_ = function() {
  /* @const */ var transitionText = '-webkit-transition';
  /* @const */ var transformText = '-webkit-transform';
  if (!microphone.isLevelAnimating_) {
    microphone.level_.style.removeProperty('opacity');
    microphone.level_.style.removeProperty(transitionText);
    microphone.level_.style.removeProperty(transformText);
    return;
  }
  /* @const */ var scale = microphone.LEVEL_SCALE_MINIMUM_ +
      Math.random() * microphone.LEVEL_SCALE_RANGE_;
  /* @const */ var timeStep = Math.round(
      microphone.LEVEL_TIME_STEP_MINIMUM_ +
      Math.random() * microphone.LEVEL_TIME_STEP_RANGE_);
  /* @const */ var transitionInterval =
      timeStep / microphone.SECOND_IN_MILLISECONDS_;
  microphone.level_.style.setProperty(
      transitionText,
      transformText + ' ' + transitionInterval + 's ease-in-out');
  microphone.level_.style.setProperty(transformText, 'scale(' + scale + ')');
  window.setTimeout(microphone.runLevelAnimation_, timeStep);
};
/* END MICROPHONE VIEW */


/* VIEW */
/**
 * The set of user aborted states.
 * @enum {string}
 * @private
 */
voice.view.ClickTarget_ = {
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
voice.view.OVERLAY_CLASS_ = 'voice overlay';


/**
 * Class name of the speech recognition interface when it is hidden on the
 * homepage.
 * @const @private
 */
voice.view.OVERLAY_HIDDEN_CLASS_ = 'voice overlay-hidden';


/**
 * ID for the speech output background.
 * @const @private
 */
voice.view.BACKGROUND_ID_ = 'voice-overlay';


/**
 * ID of the close (x) button.
 * @const @private
 */
voice.view.CLOSE_BUTTON_ID_ = 'voice-close-button';


/**
 * ID for the speech output container.
 * @const @private
 */
voice.view.CONTAINER_ID_ = 'voice-outer';


/**
 * Class name used to modify the UI to the 'listening' state.
 * @const @private
 */
voice.view.MICROPHONE_LISTENING_CLASS_ = 'outer voice-ml';


/**
 * Class name used to modify the UI to the 'receiving audio' state.
 * @const @private
 */
voice.view.RECEIVING_AUDIO_CLASS_ = 'outer voice-ra';


/**
 * Class name used to modify the UI to the 'error received' state.
 * @const @private
 */
voice.view.ERROR_RECEIVED_CLASS_ = 'outer voice-er';


/**
 * Class name used to modify the UI to the inactive state.
 * @const @private
 */
voice.view.INACTIVE_CLASS_ = 'outer';


/**
 * Background element and container of all other elements.
 * @private {Element}
 */
voice.view.background_;


/**
 * The container used to position the microphone and text output area.
 * @private {Element}
 */
voice.view.container_;


/**
 * True if the the last error message shown was for the 'no-match' error.
 * @private {boolean}
 */
voice.view.isNoMatchShown_ = false;


/**
 * True if the UI elements are visible.
 * @private {boolean}
 */
voice.view.isVisible_ = false;


/**
 * The function to call when there is a click event.
 * @private {!view.ClickHandler}
 */
voice.view.onClick_;


/**
 * Displays the UI.
 */
voice.view.show = function() {
  if (!view.isVisible_) {
    text.showInitializingMessage();
    view.showView_();
    window.addEventListener('mouseup', view.handleWindowClick_, false);
  }
};

/**
 * Sets the output area text to listening. This should only be called when
 * the Web Speech API is receiving audio input (i.e., onaudiostart).
 */
voice.view.setReadyForSpeech = function() {
  if (view.isVisible_) {
    view.container_.className = view.MICROPHONE_LISTENING_CLASS_;
    text.showReadyMessage();
  }
};


/**
 * Shows the pulsing animation emanating from the microphone. This should only
 * be called when the Web Speech API is receiving speech input (i.e.,
 * onspeechstart).
 */
voice.view.setReceivingSpeech = function() {
  if (view.isVisible_) {
    view.container_.className = view.RECEIVING_AUDIO_CLASS_;
    microphone.startInputAnimation();
    text.cancelListeningTimeout();
  }
};


/**
 * Updates the speech recognition results output with the latest results.
 * @param {string} interimResultText Low confidence recognition text (grey).
 * @param {string} finalResultText High confidence recognition text (black).
 */
voice.view.updateSpeechResult = function(interimResultText, finalResultText) {
  if (view.isVisible_) {
    view.container_.className = view.RECEIVING_AUDIO_CLASS_;
    text.updateText(interimResultText, finalResultText);
  }
};


/**
 * Hides the UI and stops animations.
 */
voice.view.hide = function() {
  window.removeEventListener('mouseup', view.handleWindowClick_, false);
  view.stopMicrophoneAnimations_();
  view.hideView_();
  view.isNoMatchShown_ = false;
  text.clear();
};


/**
 * Find the page elements that will be used to render the speech recognition
 * interface area.
 * @param {!view.ClickHandler} onClick The function to call when
 *     there is a click event in the window.
 */
voice.view.init = function(onClick) {
  view.onClick_ = onClick;

  // TODO(oskopek): Test for the presence of these elements.
  view.background_ = $(view.BACKGROUND_ID_);
  view.container_ = $(view.CONTAINER_ID_);

  text.init();
  microphone.init();
};


/**
 * Displays an error message and stops animations.
 * @param {RecognitionError} error The error type.
 */
voice.view.showError = function(error) {
  view.container_.className = view.ERROR_RECEIVED_CLASS_;
  text.updateText(text.getErrorMessage(error));
  view.stopMicrophoneAnimations_();
  if (error == RecognitionError.NO_MATCH) {
    view.isNoMatchShown_ = true;
  }
};


/**
 * Makes the view visible.
 * @private
 */
voice.view.showView_ = function() {
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
voice.view.showFullPage_ = function() {
  view.background_.className = view.OVERLAY_HIDDEN_CLASS_;
  view.background_.className = view.OVERLAY_CLASS_;
};


/**
 * Hides the view.
 * @private
 */
voice.view.hideView_ = function() {
  view.background_.className = view.OVERLAY_HIDDEN_CLASS_;
  view.container_.className = view.INACTIVE_CLASS_;
  view.background_.removeAttribute('style');
  view.background_.hidden = true;
  view.isVisible_ = false;
};


/**
 * Stops the animations in the microphone view.
 * @private
 */
voice.view.stopMicrophoneAnimations_ = function() {
  microphone.stopInputAnimation();
};


/**
 * Makes sure that a click anywhere closes the ui when it is active.
 * @param {Event} event The click event.
 * @private
 */
voice.view.handleWindowClick_ = function(event) {
  if (!view.isVisible_) {
    return;
  }
  /* @const */ var targetId = event.target.id;
  var clickTarget = view.ClickTarget_.UNKNOWN_AREA;
  var additionalData = '';
  if (targetId == view.CLOSE_BUTTON_ID_) {
    clickTarget = view.ClickTarget_.X;
  } else if (targetId == view.BACKGROUND_ID_) {
    clickTarget = view.ClickTarget_.BACKGROUND;
  } else if (targetId == microphone.RED_BUTTON_ID) {
    clickTarget = view.ClickTarget_.RED_BUTTON;
  } else if (targetId == text.ERROR_LINK_ID) {
    clickTarget = view.ClickTarget_.ERROR_LINK;
  } else {
    additionalData = targetId;
  }

  /* @const */
  var shouldRetry = (clickTarget == view.ClickTarget_.RED_BUTTON ||
                     clickTarget == view.ClickTarget_.ERROR_LINK) &&
      view.isNoMatchShown_;

  /* @const */
  var submitQuery =
      clickTarget == view.ClickTarget_.RED_BUTTON && !view.isNoMatchShown_;

  view.onClick_(clickTarget, additionalData, submitQuery, shouldRetry);
};
/* END VIEW */
