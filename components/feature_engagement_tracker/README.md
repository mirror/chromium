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

Several parts of the feature engagement tracker has some debug logging available. To see if the
current checked in code covers your needs, try starting chrome with the following command line arguments:

```bash
--vmodule=feature_engagement_tracker_impl*=2,model_impl*=2,availability_store*=2,chrome_variations_configuration*=3
```

## Using Chrome Variations

### Description of configuration options.

fdsa

### Using Chrome Variations at runtime.

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
                "event_1": "name:download_completed;comparator:>=1;window:120;storage:180",
                "event_used": "name:download_home_opened;comparator:any;window:0;storage:360",
                "event_trigger": "name:download_home_iph_trigger;comparator:any;window:0;storage:360",
                "session_rate": "<1",
                "availability": "ANY"
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
