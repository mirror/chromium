# Feature Engagement Tracker

The Feature Engagement Tracker provides a client-side backend for displaying
feature enlightenment or in-product help (IPH) with a clean and easy to use API
to be consumed by the UI frontend. The backend behaves as a black box and takes
input about user behavior. Whenever the frontend gives a trigger signal that
in-product help could be displayed, the backend will provide an answer to
whether it is appropriate to show it or not.

The frontend only needs to deal with user interactions and how to display the
feature enlightenment or in-product help itself.

The backend is feature agnostic and have no special logic for any specific
features, but instead provides a generic API.

In general, the backend is able to count named events that happen and stores
the count of each event in daily buckets. Whether In-Product Help should be
triggered is based on server side configuration.

## Testing

To compile and run tests, assuming the product out directory is `out/Debug`,
use:

```bash
ninja -C out/Debug components_unittests ;
./out/Debug/components_unittests \
  --test-launcher-filter-file=components/feature_engagement_tracker/components_unittests.filter
```

When adding new test suites, also remember to add the suite to the filter file:
`//components/feature_engagement_tracker/components_unittests.filter`.

## Printf debugging

Several parts of the feature engagement tracker has some debug logging available. To see if the current checked in code covers your needs, try starting a debug build of chrome with the following command line arguments:

```bash
--vmodule=feature_engagement_tracker_impl*=2,model_impl*=2,availability_store*=2,chrome_variations_configuration*=3
```

## Demo mode

The Feature Engagement Tracker supports a special demo mode, which enables a
developer or testers to see how the UI looks like without using Chrome
Variations configuration.

The demo mode behaves differently than the code used in production where the
chrome Variations configuration is used. Instead, it has only a few rules:

*  Event model must be ready (happens early).
*  No other features must be showing at the moment.
*  The given feature must not have been shown before in the current session.

This basically leads to each selected IPH feature to be displayed once. The triggering condition code path must of course be triggered to display the IPH.

How to select a feature or features is described below.

### Enabling all In-Product Help features in demo-mode

1.  Go to chrome://flags
1.  Find "In-Product Help Demo Mode" (#in-product-help-demo-mode-choice)
1.  Select "Enabled"
1.  Restart Chrome

### Enabling a single In-Product Help feature in demo-mode

1.  Go to chrome://flags
1.  Find “In-Product Help Demo Mode” (#enable-iph-demo-choice)
1.  Select the feature you want with the "Enabled " prefix, for example for `IPH_YourFeature` you would select:
    *  Enabled IPH_YourFeature
1.  Restart Chrome

## Using Chrome Variations

Each In-Product Help feature must have its own feature configuration [FeatureConfig](#FeatureConfig), which has 4 required configuration items that must be set, and then there can be an arbitrary number of additional preconditions (but typically on the order of 0-5).

The data types are listed below.

### FeatureConfig

Format:

```
{
  "availability": "{Comparator}",
  "session_rate": "{Comparator}",
  "event_used": "{EventConfig}",
  "event_trigger": "{EventConfig}",
  "event_???": "{EventConfig}",
 }
```

The `FeatureConfig` fields `availability`, `session_rate`, `event_used` and `event_trigger` are required, and there can be an arbitrary amount of other `event_???` entries.

* `availability`
  * For how long must an in-product help experiment have been available to the end user.
  * The value of the `Comparator` is in a number of days.
* `session_rate`
  * How many other in-product help have been displayed within the current end user session.
  * The value of the `Comparator` is a count of total In-Product Help displayed in the current end user session.
* `event_used`
  * Relates to what the in-product help wants to highlight, i.e. teach the user about and increase usage of.
  * This is typically the action that the In-Product Help should stimulate usage of.
  * Special UMA is tracked for this.
* `event_trigger`
  * Relates to the times in-product help is triggered.
  * Special UMA is tracked for this.
* `event_???`
  * Similar to the other `event_` items, but for all other preconditions that must have been met.
  * Name must match `/^event_[a-zA-Z0-9-_]+$/` and not be `event_used` or `event_trigger`.

**Examples**

```
{
  "event_1": "name:download_completed;comparator:>=1;window:135;storage:360",
  "event_used": "name:download_home_opened;comparator:==0;window:135;storage:360",
  "event_trigger": "name:download_home_iph_trigger;comparator:==0;window:90;storage:360",
  "session_rate": "<1",
  "availability": ">=1"
}
```


### EventConfig

Format: ```name:{std::string};comparator:{COMPARATOR};window:{uint32_t};storage:{uint32_t}```

The EventConfig is a semi-colon separate data structure with 4 key-value pairs, all described below:

*  `name`
   * The name (unique identifier) of the event.
   * Must match what is used in client side code.
   * Must only contain alphanumeric, dash and underscore.
    * Specifically must match this regex: `/^[a-zA-Z0-9-_]+$/`
   * Value client side data type: std::string
*  `comparator`
   * The comparator for the event. See [Comparator](#Comparator) below.
*  `window`
   * Search for this occurrences of the event within this window.
   * The value must be given as a number of days.
   * Value client side data type: uint32_t
*  `storage`
   * Store client side data related to events for this event minimum this long.
   * The value must be given as a number of days.
   * Value client side data type: uint32_t
   * Whenever a particular event is used by multiple features, the maximum value of all `storage` is used as the storage window.

**Examples**

```
name:user_opened_app_menu;comparator:==0;window:14;storage:90
name:user_has_seen_dino;comparator:>=5;window:30;storage:360
name:user_has_seen_wifi;comparator:>=1;window:30;storage:180
```

### Comparator

Format: ```{COMPARATOR}[value]```

The following comparators are allowed:

*  `<` less than
*  `>` greater than
*  `<=` less than or equal
*  `>=` greater than or equal
*  `==` equal
*  `!=` not equal
*  `any` always true (no value allowed)

Other than `any`, all comparators require a value.

**Examples**

```
>=10
==0
any
<15
```

### Using Chrome Variations at runtime

It is possible to test the whole backend from parsing the configuration,
to ensuring that help triggers at the correct time. To do that
you need to provide a JSON configuration file, that is then
parsed to become command line arguments for Chrome, and after
that you can start Chrome and verify that it behaves correctly.

1.  Create a file which describes the configuration you are planning
    on testing with, and store it. In the following example, store the
    file `DownloadStudy.json`:

    ```javascript
    {
      "DownloadStudy": [
        {
          "platforms": ["android"],
          "experiments": [
            {
              "name": "DownloadExperiment",
              "params": {
                "availability": "ANY"
                "session_rate": "<1",
                "event_used": "name:download_home_opened;comparator:any;window:0;storage:360",
                "event_trigger": "name:download_home_iph_trigger;comparator:any;window:0;storage:360",
                "event_1": "name:download_completed;comparator:>=1;window:120;storage:180",
              },
              "enable_features": ["IPH_DownloadHome"],
              "disable_features": []
            }
          ]
        }
      ]
    }
    ```

1.  Use the field trial utility to convert the JSON configuration to command
    line arguments:

    ```bash
    python ./tools/variations/fieldtrial_util.py DownloadStudy.json android shell_cmd
    ```

1.  Pass the command line along to the binary you are planning on running or the command line utility for the Android platform.

    For the target `chrome_public_apk` it would be:

    ```bash
    ./build/android/adb_chrome_public_command_line "--enable-features=IPH_DownloadHome<DownloadStudy" "--force-fieldtrials=DownloadStudy/DownloadExperiment" "--force-fieldtrial-params=DownloadStudy.DownloadExperiment:availability/ANY/event_1/name%3Adownload_completed;comparator%3A>=1;window%3A120;storage%3A180/event_trigger/name%3Adownload_home_iph_trigger;comparator%3Aany;window%3A0;storage%3A360/event_used/name%3Adownload_home_opened;comparator%3Aany;window%3A0;storage%3A360/session_rate/<1"
    ```

## Configuring UMA

To enable UMA tracking, you need to make the following changes to the metrics
configuration:

1.  Add feature to the histogram suffix `IPHFeatures` in: `//tools/metrics/histograms/histograms.xml`.
  * The suffix must match the `base::Feature` `name` member of your feature.

1.  Add feature to the actions file (actions do not support automatic suffixes): `//tools/metrics/actions/actions.xml`.
   * The suffix must match the `base::Feature` `name` member.
   * Use an old example to ensure you configure and describe it correctly.
   * For `IPH_YourFeature` it would look like this:
    *  `<action name="InProductHelp.NotifyEvent.IPH_YourFeature">`
    *  `<action name="InProductHelp.NotifyUsedEvent.IPH_YourFeature">`
    *  `<action name="InProductHelp.ShouldTriggerHelpUI.IPH_YourFeature">`
