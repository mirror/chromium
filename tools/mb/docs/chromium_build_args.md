**aec_untrusted_delay_for_testing**  Default = false
    //third_party/webrtc/modules/audio_processing/BUILD.gn:17
    Disables the usual mode where we trust the reported system delay
    values the AEC receives. The corresponding define is set appropriately
    in the code, but it can be force-enabled here for testing.

**allow_posix_link_time_opt**  Default = false
    //build/toolchain/toolchain.gni:16
    Enable Link Time Optimization in optimized builds (output programs run
    faster, but linking is up to 5-20x slower).
    Note: use target_os == "linux" rather than is_linux so that it does not
    apply to host_toolchain when target_os="android".

**android_full_debug**  Default = false
    //build/config/compiler/BUILD.gn:32
    Normally, Android builds are lightly optimized, even for debug builds, to
    keep binary size down. Setting this flag to true disables such optimization

**apm_debug_dump**  Default = false
    //third_party/webrtc/build/webrtc.gni:49
    Selects whether debug dumps for the audio processing module
    should be generated.

**asan_globals**  Default = false
    //build/config/sanitizers/sanitizers.gni:139
    Detect overflow/underflow for global objects.
   
    Mac: http://crbug.com/352073

**auto_profile_path**  Default = ""
    //build/config/compiler/BUILD.gn:90
    AFDO (Automatic Feedback Directed Optimizer) is a form of profile-guided
    optimization that GCC supports. It used by ChromeOS in their official
    builds. To use it, set auto_profile_path to the path to a file containing
    the needed gcov profiling data.

**binutils_path**  Default = "../../third_party/binutils/Linux_x64/Release/bin"
    //build/config/compiler/BUILD.gn:41

**blink_gc_plugin**  Default = true
    //third_party/WebKit/Source/BUILD.gn:17
    Set to true to enable the clang plugin that checks the usage of the  Blink
    garbage-collection infrastructure during compilation.

**blink_gc_plugin_option_do_dump_graph**  Default = false
    //third_party/WebKit/Source/BUILD.gn:21
    Set to true to have the clang Blink GC plugin emit class graph (in JSON)
    with typed pointer edges; for debugging or other (internal) uses.

**blink_gc_plugin_option_use_chromium_style_naming**  Default = false
    //third_party/WebKit/Source/BUILD.gn:31
    Set to true to have the clang Blink GC plugin use Chromium-style naming
    rather than legacy Blink name.
    TODO(https://crbug.com/675879): Remove this option after the Blink rename.

**blink_gc_plugin_option_warn_unneeded_finalizer**  Default = false
    //third_party/WebKit/Source/BUILD.gn:26
    Set to true to have the clang Blink GC plugin additionally check if
    a class has an empty destructor which would be unnecessarily invoked
    when finalized.

**blink_logging_always_on**  Default = false
    //third_party/WebKit/Source/config.gni:22
    If true, enables WTF::ScopedLogger unconditionally.
    When false, WTF::ScopedLogger is enabled only if assertions are enabled.

**build_angle_deqp_tests**  Default = false
    //third_party/angle/src/tests/BUILD.gn:204
    Don't build dEQP by default.

**build_libsrtp_tests**  Default = false
    //third_party/libsrtp/BUILD.gn:10
    Tests may not be appropriate for some build environments, e.g. Windows.
    Rather than enumerate valid options, we just let clients ask for them.

**build_sfntly_samples**  Default = false
    //third_party/sfntly/BUILD.gn:9
    Flip to true to build sfntly sample programs.

**build_with_mozilla**  Default = false
    //third_party/webrtc/build/webrtc.gni:71
    Enable to use the Mozilla internal settings.

**bundle_pool_depth**  Default = -1
    //build/toolchain/mac/BUILD.gn:28
    Reduce the number of tasks using the copy_bundle_data and compile_xcassets
    tools as they can cause lots of I/O contention when invoking ninja with a
    large number of parallel jobs (e.g. when using distributed build like goma).

**cc_wrapper**  Default = ""
    //build/toolchain/cc_wrapper.gni:36
    Set to "ccache", "icecc" or "distcc".  Probably doesn't work on windows.

**chrome_pgo_phase**  Default = 0
    //build/config/compiler/pgo/pgo.gni:13
    Specify the current PGO phase.
    Here's the different values that can be used:
        0 : Means that PGO is turned off.
        1 : Used during the PGI (instrumentation) phase.
        2 : Used during the PGO (optimization) phase.
   
    TODO(sebmarchand): Add support for the PGU (update) phase.

**clang_base_path**  Default = "//third_party/llvm-build/Release+Asserts"
    //build/config/clang/clang.gni:12

**clang_use_chrome_plugins**  Default = true
    //build/config/clang/clang.gni:10
    Indicates if the build should use the Chrome-specific plugins for enforcing
    coding guidelines, etc. Only used when compiling with Clang.

**clang_version**  Default = "4.0.0"
    //build/toolchain/toolchain.gni:53
    Clang compiler version. Clang files are placed at version-dependent paths.

**compute_grit_inputs_for_analyze**  Default = false
    //tools/grit/grit_rule.gni:108
    When set, this will fork out to grit at GN-time to get all inputs
    referenced by the .grd file.
   
    Normal builds don't need this since proper incremental builds are handled
    by grit writing out the inputs in .d files at build-time. But for analyze
    to work on the bots, it needs to know about all input files at GN-time so
    it knows which tests to trigger when something changes. This option will
    slow down the GN run.

**concurrent_links**  Default = -1
    //build/toolchain/concurrent_links.gni:19
    Limit the number of concurrent links; we often want to run fewer
    links at once than we do compiles, because linking is memory-intensive.
    The default to use varies by platform and by the amount of memory
    available, so we call out to a script to get the right value.

**content_shell_product_name**  Default = "Content Shell"
    //content/shell/BUILD.gn:27

**content_shell_version**  Default = "99.77.34.5"
    //content/shell/BUILD.gn:28

**current_cpu**  Default = ""
    (Internally set; try `gn help current_cpu`.)

**current_os**  Default = ""
    (Internally set; try `gn help current_os`.)

**custom_toolchain**  Default = ""
    //build/config/BUILDCONFIG.gn:143
    Allows the path to a custom target toolchain to be injected as a single
    argument, and set as the default toolchain.

**dcheck_always_on**  Default = false
    //build/config/dcheck_always_on.gni:7
    Set to true to enable dcheck in Release builds.

**debug_devtools**  Default = false
    //third_party/WebKit/public/public_features.gni:12
    If debug_devtools is set to true, JavaScript files for DevTools are stored
    as is and loaded from disk. Otherwise, a concatenated file is stored in
    resources.pak. It is still possible to load JS files from disk by passing
    --debug-devtools cmdline switch.

**disable_brotli_filter**  Default = false
    //net/features.gni:22
    Do not disable brotli filter by default.

**disable_file_support**  Default = false
    //net/features.gni:9
    Disables support for file URLs.  File URL support requires use of icu.

**disable_ftp_support**  Default = false
    //net/features.gni:14

**disable_libfuzzer**  Default = false
    //build/config/sanitizers/sanitizers.gni:86
    Helper variable for testing builds with disabled libfuzzer.
    Not for client use.

**enable_ac3_eac3_audio_demuxing**  Default = false
    //media/media_options.gni:39
    Enables AC3/EAC3 audio demuxing. This is enabled only on Chromecast, since
    it only provides demuxing, and is only useful for AC3/EAC3 audio
    pass-through to HDMI sink on Chromecast.

**enable_app_list**  Default = false
    //chrome/common/features.gni:18

**enable_background**  Default = true
    //chrome/common/features.gni:21
    Enables support for background apps.

**enable_basic_print_dialog**  Default = true
    //chrome/common/features.gni:25
    Enable the printing system dialog for platforms that support printing
    and have a system dialog.

**enable_basic_printing**  Default = true
    //printing/features/features.gni:10
    Enable basic printing support and UI.

**enable_captive_portal_detection**  Default = true
    //chrome/common/features.gni:27

**enable_cbcs_encryption_scheme**  Default = false
    //media/media_options.gni:46
    Enable support for the 'cbcs' encryption scheme added by MPEG Common
    Encryption 3rd Edition (ISO/IEC 23001-7), published 02/15/2016.

**enable_cross_trusted**  Default = false
    //native_client_sdk/src/BUILD.gn:10
    Set to true if cross compiling trusted (e.g. building sel_ldr_arm on x86)
    binaries is supported.

**enable_debugallocation**  Default = true
    //base/allocator/BUILD.gn:13
    Provide a way to force disable debugallocation in Debug builds,
    e.g. for profiling (it's more rare to profile Debug builds,
    but people sometimes need to do that).

**enable_dsyms**  Default = false
    //build/config/mac/symbols.gni:17
    Produce dSYM files for targets that are configured to do so. dSYM
    generation is controlled globally as it is a linker output (produced via
    the //build/toolchain/mac/linker_driver.py. Enabling this will result in
    all shared library, loadable module, and executable targets having a dSYM
    generated.

**enable_extensions**  Default = true
    //extensions/features/features.gni:8

**enable_full_stack_frames_for_profiling**  Default = false
    //build/config/compiler/BUILD.gn:49
    Compile in such a way as to make it possible for the profiler to unwind full
    stack frames. Setting this flag has a large effect on the performance of the
    generated code than just setting profiling, but gives the profiler more
    information to analyze.
    Requires profiling to be set to true.

**enable_google_now**  Default = false
    //chrome/common/features.gni:31
    Google Now is disabled to prepare for its removal.
    http://crbug.com/539674

**enable_hangout_services_extension**  Default = false
    //chrome/common/features.gni:35
    Hangout services is an extension that adds extra features to Hangouts.
    It is enableable separately to facilitate testing.

**enable_hevc_demuxing**  Default = false
    //media/media_options.gni:50
    Enable HEVC/H265 demuxing. Actual decoding must be provided by the
    platform. Enable by default for Chromecast.

**enable_hls_sample_aes**  Default = false
    //media/media_options.gni:57
    Enable HLS with SAMPLE-AES decryption.
    Enabled by default on the cast desktop implementation to allow unit tests of
    MP2TS parsing support.

**enable_hotwording**  Default = false
    //chrome/common/features.gni:40
    'Ok Google' hotwording is disabled by default. Set to true to enable. (This
    will download a closed-source NaCl module at startup.) Chrome-branded
    ChromeOS builds have this enabled by default.

**enable_internal_app_remoting_targets**  Default = false
    //remoting/remoting_options.gni:13
    Set this to enable building internal AppRemoting apps.

**enable_ipc_fuzzer**  Default = false
    //tools/ipc_fuzzer/ipc_fuzzer.gni:14

**enable_iterator_debugging**  Default = true
    //build/config/BUILD.gn:34
    When set (the default) enables C++ iterator debugging in debug builds.
    Iterator debugging is always off in release builds (technically, this flag
    affects the "debug" config, which is always available but applied by
    default only in debug builds).
   
    Iterator debugging is generally useful for catching bugs. But it can
    introduce extra locking to check the state of an iterator against the state
    of the current object. For iterator- and thread-heavy code, this can
    significantly slow execution.

**enable_linux_installer**  Default = false
    //chrome/installer/BUILD.gn:8

**enable_mdns**  Default = false
    //net/features.gni:25
    Multicast DNS.

**enable_media_codec_video_decoder**  Default = false
    //media/gpu/BUILD.gn:14
    A temporary arg for building MCVD while it's being implemented.
    See http://crbug.com/660942

**enable_media_remoting**  Default = true
    //media/media_options.gni:145
    This switch defines whether the Media Remoting implementation will be built.
    When enabled, media is allowed to be renderer and played back on remote
    devices when the tab is being casted and other conditions are met.

**enable_media_router**  Default = true
    //build/config/features.gni:34
    Enables the Media Router.

**enable_memory_task_profiler**  Default = false
    //base/BUILD.gn:43

**enable_mojo_media**  Default = false
    //media/media_options.gni:89
    Experiment to enable mojo media services (e.g. "renderer", "cdm", see
    |mojo_media_services|). When enabled, selected mojo paths will be enabled in
    the media pipeline and corresponding services will hosted in the selected
    remote process (e.g. "utility" process, see |mojo_media_host|).

**enable_mse_mpeg2ts_stream_parser**  Default = false
    //media/media_options.gni:42

**enable_nacl**  Default = true
    //build/config/features.gni:27
    Enables Native Client support.
    Temporarily disable nacl on arm64 linux to get rid of compilation errors.
    TODO(mcgrathr): When mipsel-nacl-clang is available, drop the exclusion.

**enable_nacl_nonsfi**  Default = true
    //build/config/features.gni:31
    Non-SFI is not yet supported on mipsel

**enable_nocompile_tests**  Default = false
    //build/nocompile.gni:62
    TODO(crbug.com/105388): make sure no-compile test is not flaky.

**enable_one_click_signin**  Default = true
    //chrome/common/features.gni:43

**enable_package_mash_services**  Default = false
    //chrome/common/features.gni:47
    Set to true to bundle all the mash related mojo services into chrome.
    Specify --mash to chrome to have chrome start the mash environment.

**enable_pdf**  Default = true
    //pdf/features.gni:12

**enable_plugin_installation**  Default = true
    //chrome/common/features.gni:49

**enable_plugins**  Default = true
    //ppapi/features/features.gni:9

**enable_precompiled_headers**  Default = true
    //build/config/pch.gni:11
    Precompiled header file support is by default available,
    but for distributed build system uses (like goma) or when
    doing official builds.

**enable_print_preview**  Default = true
    //printing/features/features.gni:14
    Enable printing with print preview. It does not imply
    enable_basic_printing. It's possible to build Chrome with preview only.

**enable_profiling**  Default = false
    //build/config/compiler/compiler.gni:25
    Compile in such a way as to enable profiling of the generated code. For
    example, don't omit the frame pointer and leave in symbols.

**enable_reading_list**  Default = true
    //components/reading_list/core/reading_list.gni:8
    Controls whether reading list support is active or not. Currently only
    supported on iOS (on other platforms, the feature is always disabled).

**enable_remoting**  Default = true
    //remoting/remoting_enable.gni:8

**enable_remoting_jscompile**  Default = false
    //remoting/remoting_options.gni:10
    Set this to run the jscompile checks after building the webapp.

**enable_resource_whitelist_generation**  Default = false
    //build/config/android/config.gni:353
    Enables used resource whitelist generation. Set for official builds only
    as a large amount of build output is generated.

**enable_service_discovery**  Default = true
    //chrome/common/features.gni:51

**enable_session_service**  Default = true
    //chrome/common/features.gni:55
    Enables use of the session service, which is enabled by default.
    Android stores them separately on the Java side.

**enable_stripping**  Default = false
    //build/config/mac/symbols.gni:24
    Strip symbols from linked targets by default. If this is enabled, the
    //build/config/mac:strip_all config will be applied to all linked targets.
    If custom stripping parameters are required, remove that config from a
    linked target and apply custom -Wcrl,strip flags. See
    //build/toolchain/mac/linker_driver.py for more information.

**enable_supervised_users**  Default = true
    //chrome/common/features.gni:57

**enable_swiftshader**  Default = false
    //ui/gl/BUILD.gn:13

**enable_test_mojo_media_client**  Default = false
    //media/media_options.gni:93
    Enable the TestMojoMediaClient to be used in mojo MediaService. This is for
    testing only and will override the default platform MojoMediaClient, if any.

**enable_vr_shell**  Default = false
    //chrome/common/features.gni:62

**enable_vr_shell_ui_dev**  Default = false
    //chrome/common/features.gni:65
    Enables vr shell UI development on local or remote page.

**enable_vulkan**  Default = false
    //build/config/ui.gni:43
    Enable experimental vulkan backend.

**enable_wayland_server**  Default = false
    //build/config/ui.gni:40
    Indicates if Wayland display server support is enabled.

**enable_webrtc**  Default = true
    //media/media_options.gni:52

**enable_websockets**  Default = true
    //net/features.gni:13
    WebSockets and socket stream code are not used on iOS and are optional in
    cronet.

**enable_webvr**  Default = false
    //build/config/features.gni:75
    Enable WebVR support by default on Android
    Still requires command line flag to access API
    TODO(bshe): Enable for other architecture too. Currently we only support arm
    and arm64.

**enable_widevine**  Default = false
    //third_party/widevine/cdm/widevine.gni:7
    Allow widevinecdmadapter to be built in Chromium.

**enable_wifi_display**  Default = false
    //extensions/features/features.gni:12
    Enables Wi-Fi Display functionality
    WARNING: This enables MPEG Transport Stream (MPEG-TS) encoding!

**enable_xpc_notifications**  Default = false
    //chrome/common/features.gni:68
    Enable native notifications via XPC services (mac only).

**exclude_unwind_tables**  Default = false
    //build/config/compiler/BUILD.gn:64
    Omit unwind support in official builds to save space.
    We can use breakpad for these builds.

**fatal_linker_warnings**  Default = true
    //build/config/compiler/BUILD.gn:79
    Enable fatal linker warnings. Building Chromium with certain versions
    of binutils can cause linker warning.
    See: https://bugs.chromium.org/p/chromium/issues/detail?id=457359

**ffmpeg_branding**  Default = "Chromium"
    //third_party/ffmpeg/ffmpeg_options.gni:34
    Controls whether we build the Chromium or Google Chrome version of FFmpeg.
    The Google Chrome version contains additional codecs. Typical values are
    Chromium, Chrome, and ChromeOS.

**fieldtrial_testing_like_official_build**  Default = false
    //build/config/features.gni:55
    Set to true make a build that disables activation of field trial tests
    specified in testing/variations/fieldtrial_testing_config_*.json.
    Note: this setting is ignored if is_chrome_branded.

**full_wpo_on_official**  Default = false
    //build/config/compiler/compiler.gni:69

**gcc_target_rpath**  Default = ""
    //build/config/gcc/BUILD.gn:18
    When non empty, overrides the target rpath value. This allows a user to
    make a Chromium build where binaries and shared libraries are meant to be
    installed into separate directories, like /usr/bin/chromium and
    /usr/lib/chromium for instance. It is useful when a build system that
    generates a whole target root filesystem (like Yocto) is used on top of gn,
    especially when cross-compiling.
    Note: this gn arg is similar to gyp target_rpath generator flag.

**gdb_index**  Default = false
    //build/config/compiler/BUILD.gn:68
    If true, gold linker will save symbol table inside object files.
    This speeds up gdb startup by 60%

**gold_path**  Default = false
    //build/config/compiler/BUILD.gn:53
    When we are going to use gold we need to find it.
    This is initialized below, after use_gold might have been overridden.

**goma_dir**  Default = "/Users/dpranke/goma"
    //build/toolchain/goma.gni:17
    Absolute directory containing the gomacc binary.

**google_api_key**  Default = ""
    //google_apis/BUILD.gn:46
    Set these to bake the specified API keys and OAuth client
    IDs/secrets into your build.
   
    If you create a build without values baked in, you can instead
    set environment variables to provide the keys at runtime (see
    src/google_apis/google_api_keys.h for details).  Features that
    require server-side APIs may fail to work if no keys are
    provided.
   
    Note that if you are building an official build or if
    use_official_google_api_keys has been set to trie (explicitly or
    implicitly), these values will be ignored and the official
    keys will be used instead.

**google_default_client_id**  Default = ""
    //google_apis/BUILD.gn:49
    See google_api_key.

**google_default_client_secret**  Default = ""
    //google_apis/BUILD.gn:52
    See google_api_key.

**host_cpu**  Default = "x64"
    (Internally set; try `gn help host_cpu`.)

**host_os**  Default = "mac"
    (Internally set; try `gn help host_os`.)

**host_toolchain**  Default = ""
    //build/config/BUILDCONFIG.gn:147
    This should not normally be set as a build argument.  It's here so that
    every toolchain can pass through the "global" value via toolchain_args().

**icu_use_data_file**  Default = true
    //third_party/icu/config.gni:15
    Tells icu to load an external data file rather than rely on the icudata
    being linked directly into the binary.
   
    This flag is a bit confusing. As of this writing, icu.gyp set the value to
    0 but common.gypi sets the value to 1 for most platforms (and the 1 takes
    precedence).
   
    TODO(GYP) We'll probably need to enhance this logic to set the value to
    true or false in similar circumstances.

**ignore_elf32_limitations**  Default = false
    //build_overrides/build.gni:47
    Android 32-bit non-component, non-clang builds cannot have symbol_level=2
    due to 4GiB file size limit, see https://crbug.com/648948.
    Set this flag to true to skip the assertion.

**internal_gles2_conform_tests**  Default = false
    //gpu/gles2_conform_support/BUILD.gn:7
    Set to true to compile with the OpenGL ES 2.0 conformance tests.

**internal_khronos_glcts_tests**  Default = false
    //gpu/khronos_glcts_support/BUILD.gn:8

**is_asan**  Default = false
    //build/config/sanitizers/sanitizers.gni:10
    Compile for Address Sanitizer to find memory bugs.

**is_cast_audio_only**  Default = false
    //build/config/chromecast_build.gni:14
    Set this true for an audio-only Chromecast build.

**is_cast_desktop_build**  Default = false
    //build/config/chromecast_build.gni:26
    True if Chromecast build is targeted for linux desktop. This type of build
    is useful for testing and development, but currently supports only a subset
    of Cast functionality. Though this defaults to true for x86 Linux devices,
    this should be overriden manually for an embedded x86 build.
    TODO(slan): Remove instances of this when x86 is a fully supported platform.

**is_cfi**  Default = false
    //build/config/sanitizers/sanitizers.gni:57
    Compile with Control Flow Integrity to protect virtual calls and casts.
    See http://clang.llvm.org/docs/ControlFlowIntegrity.html
   
    TODO(pcc): Remove this flag if/when CFI is enabled in all official builds.

**is_chrome_branded**  Default = false
    //build/config/chrome_build.gni:9
    Select the desired branding flavor. False means normal Chromium branding,
    true means official Google Chrome branding (requires extra Google-internal
    resources).

**is_chromecast**  Default = false
    //build/config/chromecast_build.gni:11
    Set this true for a Chromecast build. Chromecast builds are supported on
    Linux and Android.

**is_clang**  Default = true
    //build/config/BUILDCONFIG.gn:138
    Set to true when compiling with the Clang compiler. Typically this is used
    to configure warnings.

**is_component_build**  Default = true
    //build/config/BUILDCONFIG.gn:164
    Component build. Setting to true compiles targets declared as "components"
    as shared libraries loaded dynamically. This speeds up development time.
    When false, components will be linked statically.
   
    For more information see
    https://chromium.googlesource.com/chromium/src/+/master/docs/component_build.md

**is_component_ffmpeg**  Default = true
    //third_party/ffmpeg/ffmpeg_options.gni:41
    Set true to build ffmpeg as a shared library. NOTE: this means we should
    always consult is_component_ffmpeg instead of is_component_build for
    ffmpeg targets. This helps linux chromium packagers that swap out our
    ffmpeg.so with their own. See discussion here
    https://groups.google.com/a/chromium.org/forum/#!msg/chromium-packagers/R5rcZXWxBEQ/B6k0zzmJbvcJ

**is_debug**  Default = true
    //build/config/BUILDCONFIG.gn:154
    Debug build. Enabling official builds automatically sets is_debug to false.

**is_desktop_linux**  Default = false
    //build/config/BUILDCONFIG.gn:134
    Whether we're a traditional desktop unix.

**is_lsan**  Default = false
    //build/config/sanitizers/sanitizers.gni:13
    Compile for Leak Sanitizer to find leaks.

**is_msan**  Default = false
    //build/config/sanitizers/sanitizers.gni:16
    Compile for Memory Sanitizer to find uninitialized reads.

**is_multi_dll_chrome**  Default = false
    //build/config/chrome_build.gni:13
    Break chrome.dll into multple pieces based on process type. Only available
    on Windows.

**is_nacl_glibc**  Default = false
    //build/config/nacl/config.gni:11
    Native Client supports both Newlib and Glibc C libraries where Newlib
    is assumed to be the default one; use this to determine whether Glibc
    is being used instead.

**is_official_build**  Default = false
    //build/config/BUILDCONFIG.gn:131
    Set to enable the official build level of optimization. This has nothing
    to do with branding, but enables an additional level of optimization above
    release (!is_debug). This might be better expressed as a tri-state
    (debug, release, official) but for historical reasons there are two
    separate flags.

**is_syzyasan**  Default = false
    //build/config/sanitizers/sanitizers.gni:51
    Enable building with SyzyAsan which can find certain types of memory
    errors. Only works on Windows. See
    https://github.com/google/syzygy/wiki/SyzyASanHowTo

**is_tsan**  Default = false
    //build/config/sanitizers/sanitizers.gni:19
    Compile for Thread Sanitizer to find threading bugs.

**is_ubsan**  Default = false
    //build/config/sanitizers/sanitizers.gni:23
    Compile for Undefined Behaviour Sanitizer to find various types of
    undefined behaviour (excludes vptr checks).

**is_ubsan_no_recover**  Default = false
    //build/config/sanitizers/sanitizers.gni:26
    Halt the program if a problem is detected.

**is_ubsan_null**  Default = false
    //build/config/sanitizers/sanitizers.gni:29
    Compile for Undefined Behaviour Sanitizer's null pointer checks.

**is_ubsan_security**  Default = false
    //build/config/sanitizers/sanitizers.gni:78
    Enables core ubsan security features. Will later be removed once it matches
    is_ubsan.

**is_ubsan_vptr**  Default = false
    //build/config/sanitizers/sanitizers.gni:32
    Compile for Undefined Behaviour Sanitizer's vptr checks.

**is_win_fastlink**  Default = false
    //build/config/compiler/compiler.gni:42
    Tell VS to create a PDB that references information in .obj files rather
    than copying it all. This should improve linker performance. mspdbcmf.exe
    can be used to convert a fastlink pdb to a normal one.

**libyuv_disable_jpeg**  Default = false
    //third_party/libyuv/libyuv.gni:15

**libyuv_include_tests**  Default = false
    //third_party/libyuv/libyuv.gni:14

**libyuv_use_msa**  Default = false
    //third_party/libyuv/libyuv.gni:18

**libyuv_use_neon**  Default = false
    //third_party/libyuv/libyuv.gni:16

**link_pulseaudio**  Default = false
    //media/media_options.gni:13
    Allows distributions to link pulseaudio directly (DT_NEEDED) instead of
    using dlopen. This helps with automated detection of ABI mismatches and
    prevents silent errors.

**linkrepro_root_dir**  Default = ""
    //build/config/compiler/compiler.gni:60
    Root directory that will store the MSVC link repro. This should only be
    used for debugging purposes on the builders where a MSVC linker flakyness
    has been observed. The targets for which a link repro should be generated
    should add somethink like this to their configuration:
      if (linkrepro_root_dir != "") {
        ldflags = ["/LINKREPRO:" + linkrepro_root_dir + "/" + target_name]
      }
   
    Note that doing a link repro uses a lot of disk space and slows down the
    build, so this shouldn't be enabled on too many targets.
   
    See crbug.com/669854.

**linux_use_bundled_binutils**  Default = false
    //build/config/compiler/BUILD.gn:39

**llvm_force_head_revision**  Default = false
    //build/toolchain/toolchain.gni:37
    If this is set to true, or if LLVM_FORCE_HEAD_REVISION is set to 1
    in the environment, we use the revision in the llvm repo to determine
    the CLANG_REVISION to use, instead of the version hard-coded into
    //tools/clang/scripts/update.py. This should only be used in
    conjunction with setting LLVM_FORCE_HEAD_REVISION in the
    environment when `gclient runhooks` is run as well.

**mac_deployment_target**  Default = "10.9"
    //build/config/mac/mac_sdk.gni:18
    Minimum supported version of OSX.

**mac_sdk_min**  Default = "10.10"
    //build/config/mac/mac_sdk.gni:15
    Minimum supported version of the Mac SDK.

**mac_sdk_name**  Default = "macosx"
    //build/config/mac/mac_sdk.gni:26
    The SDK name as accepted by xcodebuild.

**mac_sdk_path**  Default = ""
    //build/config/mac/mac_sdk.gni:23
    Path to a specific version of the Mac SDK, not including a slash at the end.
    If empty, the path to the lowest version greater than or equal to
    mac_sdk_min is used.

**mac_views_browser**  Default = false
    //ui/base/ui_features.gni:10
    Whether the entire browser uses toolkit-views on Mac instead of Cocoa.

**media_use_ffmpeg**  Default = true
    //media/media_options.gni:18
    Enable usage of FFmpeg within the media library. Used for most software
    based decoding, demuxing, and sometimes optimized FFTs. If disabled,
    implementors must provide their own demuxers and decoders.

**media_use_libvpx**  Default = true
    //media/media_options.gni:22
    Enable usage of libvpx within the media library. Used for software based
    decoding of VP9 and VP8A type content.

**metrics_use_blimp**  Default = false
    //components/metrics/BUILD.gn:7
    Overrides os name in uma metrics log to "Blimp".

**mojo_media_host**  Default = "none"
    //media/media_options.gni:116
    The process to host the mojo media service.
    Valid options are:
    - "none": Do not use mojo media service.
    - "browser": Use mojo media service hosted in the browser process.
    - "gpu": Use mojo media service hosted in the gpu process.
    - "utility": Use mojo media service hosted in the utility process.

**mojo_media_services**  Default = []
    //media/media_options.gni:108
    A list of mojo media services that should be used in the media pipeline.
    Must not be empty if |enable_mojo_media| is true.
    Valid entries in the list are:
    - "renderer": Use mojo-based media Renderer service.
    - "cdm": Use mojo-based Content Decryption Module.
    - "audio_decoder": Use mojo-based audio decoder in the default media
                       Renderer. Cannot be used with the mojo Renderer above.
    - "video_decoder": Use mojo-based video decoder in the default media
                       Renderer. Cannot be used with the mojo Renderer above.

**msan_track_origins**  Default = 2
    //build/config/sanitizers/sanitizers.gni:37
    Track where uninitialized memory originates from. From fastest to slowest:
    0 - no tracking, 1 - track only the initial allocation site, 2 - track the
    chain of stores leading from allocation site to use site.

**nacl_sdk_untrusted**  Default = false
    //native_client_sdk/src/BUILD.gn:14
    Build the nacl SDK untrusted components.  This is disabled by default since
    not all NaCl untrusted compilers are in goma (e.g arm-nacl-glibc)

**openmax_big_float_fft**  Default = true
    //third_party/openmax_dl/dl/BUILD.gn:10
    Override this value to build with small float FFT tables

**optimize_for_fuzzing**  Default = false
    //build/config/compiler/BUILD.gn:94
    Optimize for coverage guided fuzzing (balance between speed and number of
    branches)

**optimize_for_size**  Default = false
    //build/config/compiler/BUILD.gn:74
    If true, optimize for size. Does not affect windows builds.
    Linux & Mac favor speed over size.
    TODO(brettw) it's weird that Mac and desktop Linux are different. We should
    explore favoring size over speed in this case as well.

**override_build_date**  Default = "N/A"
    //base/BUILD.gn:38
    Override this value to give a specific build date.
    See //base/build_time.cc and //build/write_build_date_header.py for more
    details and the expected format.

**ozone_auto_platforms**  Default = false
    //ui/ozone/ozone.gni:10
    Select platforms automatically. Turn this off for manual control.

**ozone_platform**  Default = ""
    //ui/ozone/ozone.gni:22
    The platform that will be active by default.

**ozone_platform_cast**  Default = false
    //ui/ozone/ozone.gni:25
    Enable individual platforms.

**ozone_platform_gbm**  Default = false
    //ui/ozone/ozone.gni:26

**ozone_platform_headless**  Default = false
    //ui/ozone/ozone.gni:27

**ozone_platform_wayland**  Default = false
    //ui/ozone/ozone.gni:29

**ozone_platform_x11**  Default = false
    //ui/ozone/ozone.gni:28

**packages_directory**  Default = "Packages"
    //services/service_manager/public/constants.gni:7
    Service package directories are created within this subdirectory.

**pdf_enable_v8**  Default = true
    //third_party/pdfium/pdfium.gni:15
    Build PDFium either with or without v8 support.

**pdf_enable_xfa**  Default = false
    //third_party/pdfium/pdfium.gni:18
    Build PDFium either with or without XFA Forms support.

**pdf_is_standalone**  Default = false
    //third_party/pdfium/pdfium.gni:30
    Build PDFium standalone

**pdf_use_skia**  Default = false
    //third_party/pdfium/pdfium.gni:21
    Build PDFium against skia (experimental) rather than agg. Use Skia to draw everything.

**pdf_use_skia_paths**  Default = false
    //third_party/pdfium/pdfium.gni:24
    Build PDFium against skia (experimental) rather than agg. Use Skia to draw paths.

**pdf_use_win32_gdi**  Default = true
    //third_party/pdfium/pdfium.gni:27
    Build PDFium with or without experimental win32 GDI APIs.

**pdfium_bundle_freetype**  Default = true
    //third_party/pdfium/pdfium.gni:12
    On Android there's no system FreeType. On Windows and Mac, only a few
    methods are used from it.

**pgo_build**  Default = false
    //chrome/common/features.gni:71
    Indicates if the build is using PGO.

**pgo_data_path**  Default = ""
    //build/config/compiler/pgo/pgo.gni:16
    When using chrome_pgo_phase = 2, read profile data from this path.

**pkg_config**  Default = ""
    //build/config/linux/pkg_config.gni:33
    A pkg-config wrapper to call instead of trying to find and call the right
    pkg-config directly. Wrappers like this are common in cross-compilation
    environments.
    Leaving it blank defaults to searching PATH for 'pkg-config' and relying on
    the sysroot mechanism to find the right .pc files.

**proprietary_codecs**  Default = false
    //build/config/features.gni:38
    Enables proprietary codecs and demuxers; e.g. H264, AAC, MP3, and MP4.
    We always build Google Chrome and Chromecast with proprietary codecs.

**remove_webcore_debug_symbols**  Default = false
    //third_party/WebKit/Source/config.gni:18
    If true, doesn't compile debug symbols into webcore reducing the
    size of the binary and increasing the speed of gdb.

**root_extra_deps**  Default = []
    //BUILD.gn:33
    A list of extra dependencies to add to the root target. This allows a
    checkout to add additional targets without explicitly changing any checked-
    in files.

**rtc_build_expat**  Default = true
    //third_party/webrtc/build/webrtc.gni:58
    Disable these to not build components which can be externally provided.

**rtc_build_json**  Default = true
    //third_party/webrtc/build/webrtc.gni:59

**rtc_build_libevent**  Default = false
    //third_party/webrtc/build/webrtc.gni:90

**rtc_build_libjpeg**  Default = true
    //third_party/webrtc/build/webrtc.gni:60

**rtc_build_libsrtp**  Default = true
    //third_party/webrtc/build/webrtc.gni:61

**rtc_build_libvpx**  Default = true
    //third_party/webrtc/build/webrtc.gni:62

**rtc_build_libyuv**  Default = true
    //third_party/webrtc/build/webrtc.gni:64

**rtc_build_openmax_dl**  Default = true
    //third_party/webrtc/build/webrtc.gni:65

**rtc_build_opus**  Default = true
    //third_party/webrtc/build/webrtc.gni:66

**rtc_build_ssl**  Default = true
    //third_party/webrtc/build/webrtc.gni:67

**rtc_build_usrsctp**  Default = true
    //third_party/webrtc/build/webrtc.gni:68

**rtc_build_with_neon**  Default = false
    //third_party/webrtc/build/webrtc.gni:109

**rtc_enable_android_opensl**  Default = false
    //third_party/webrtc/build/webrtc.gni:73

**rtc_enable_bwe_test_logging**  Default = false
    //third_party/webrtc/build/webrtc.gni:52
    Set this to true to enable BWE test logging.

**rtc_enable_external_auth**  Default = true
    //third_party/webrtc/build/webrtc.gni:45
    Enable when an external authentication mechanism is used for performing
    packet authentication for RTP packets instead of libsrtp.

**rtc_enable_intelligibility_enhancer**  Default = false
    //third_party/webrtc/build/webrtc.gni:41
    Disable the code for the intelligibility enhancer by default.

**rtc_enable_libevent**  Default = false
    //third_party/webrtc/build/webrtc.gni:89

**rtc_enable_protobuf**  Default = true
    //third_party/webrtc/build/webrtc.gni:38
    Enables the use of protocol buffers for debug recordings.

**rtc_enable_sctp**  Default = true
    //third_party/webrtc/build/webrtc.gni:55
    Set this to disable building with support for SCTP data channels.

**rtc_include_ilbc**  Default = false
    //third_party/webrtc/build/webrtc.gni:149
    Include the iLBC audio codec?

**rtc_include_internal_audio_device**  Default = false
    //third_party/webrtc/build/webrtc.gni:158
    Chromium uses its own IO handling, so the internal ADM is only built for
    standalone WebRTC.

**rtc_include_opus**  Default = true
    //third_party/webrtc/build/webrtc.gni:18
    Disable this to avoid building the Opus audio codec.

**rtc_include_pulse_audio**  Default = false
    //third_party/webrtc/build/webrtc.gni:154
    Excluded in Chromium since its prerequisites don't require Pulse Audio.

**rtc_include_tests**  Default = false
    //third_party/webrtc/build/webrtc.gni:161
    Include tests in standalone checkout.

**rtc_initialize_ffmpeg**  Default = false
    //third_party/webrtc/build/webrtc.gni:137
    FFmpeg must be initialized for |H264DecoderImpl| to work. This can be done
    by WebRTC during |H264DecoderImpl::InitDecode| or externally. FFmpeg must
    only be initialized once. Projects that initialize FFmpeg externally, such
    as Chromium, must turn this flag off so that WebRTC does not also
    initialize.

**rtc_jsoncpp_root**  Default = "//third_party/jsoncpp/source/include"
    //third_party/webrtc/build/webrtc.gni:28
    Used to specify an external Jsoncpp include path when not compiling the
    library that comes with WebRTC (i.e. rtc_build_json == 0).

**rtc_libvpx_build_vp9**  Default = true
    //third_party/webrtc/build/webrtc.gni:63

**rtc_opus_variable_complexity**  Default = false
    //third_party/webrtc/build/webrtc.gni:21
    Enable this to let the Opus audio codec change complexity on the fly.

**rtc_prefer_fixed_point**  Default = false
    //third_party/webrtc/build/webrtc.gni:35
    Selects fixed-point code where possible.

**rtc_relative_path**  Default = true
    //third_party/webrtc/build/webrtc.gni:24
    Disable to use absolute header paths for some libraries.

**rtc_restrict_logging**  Default = true
    //third_party/webrtc/build/webrtc.gni:151

**rtc_sanitize_coverage**  Default = ""
    //third_party/webrtc/build/webrtc.gni:85
    Set to "func", "block", "edge" for coverage generation.
    At unit test runtime set UBSAN_OPTIONS="coverage=1".
    It is recommend to set include_examples=0.
    Use llvm's sancov -html-report for human readable reports.
    See http://clang.llvm.org/docs/SanitizerCoverage.html .

**rtc_ssl_root**  Default = ""
    //third_party/webrtc/build/webrtc.gni:32
    Used to specify an external OpenSSL include path when not compiling the
    library that comes with WebRTC (i.e. rtc_build_ssl == 0).

**rtc_use_dummy_audio_file_devices**  Default = false
    //third_party/webrtc/build/webrtc.gni:126
    By default, use normal platform audio support or dummy audio, but don't
    use file-based audio playout and record.

**rtc_use_gtk**  Default = false
    //third_party/webrtc/build/webrtc.gni:141
    Build sources requiring GTK. NOTICE: This is not present in Chrome OS
    build environments, even if available for Chromium builds.

**rtc_use_h264**  Default = false
    //third_party/webrtc/build/webrtc.gni:119
    Enable this to build OpenH264 encoder/FFmpeg decoder. This is supported on
    all platforms except Android and iOS. Because FFmpeg can be built
    with/without H.264 support, |ffmpeg_branding| has to separately be set to a
    value that includes H.264, for example "Chrome". If FFmpeg is built without
    H.264, compilation succeeds but |H264DecoderImpl| fails to initialize. See
    also: |rtc_initialize_ffmpeg|.
    CHECK THE OPENH264, FFMPEG AND H.264 LICENSES/PATENTS BEFORE BUILDING.
    http://www.openh264.org, https://www.ffmpeg.org/

**rtc_use_lto**  Default = false
    //third_party/webrtc/build/webrtc.gni:78
    Link-Time Optimizations.
    Executes code generation at link-time instead of compile-time.
    https://gcc.gnu.org/wiki/LinkTimeOptimization

**rtc_use_memcheck**  Default = false
    //third_party/webrtc/build/webrtc.gni:130
    When set to true, test targets will declare the files needed to run memcheck
    as data dependencies. This is to enable memcheck execution on swarming bots.

**rtc_use_openmax_dl**  Default = true
    //third_party/webrtc/build/webrtc.gni:102

**rtc_use_quic**  Default = false
    //third_party/webrtc/build/webrtc.gni:122
    Determines whether QUIC code will be built.

**safe_browsing_mode**  Default = 1
    //build/config/features.gni:49

**sanitizer_coverage_flags**  Default = ""
    //build/config/sanitizers/sanitizers.gni:94
    Value for -fsanitize-coverage flag. Setting this causes
    use_sanitizer_coverage to be enabled.
    Default value when unset and use_afl=true:
    trace-pc
    Default value when unset and use_sanitizer_coverage=true:
        edge,indirect-calls,8bit-counters

**skia_whitelist_serialized_typefaces**  Default = false
    //skia/BUILD.gn:23

**symbol_level**  Default = -1
    //build/config/compiler/compiler.gni:21
    How many symbols to include in the build. This affects the performance of
    the build since the symbols are large and dealing with them is slow.
      2 means regular build with symbols.
      1 means minimal symbols, usually enough for backtraces only.
      0 means no symbols.
      -1 means auto-set according to debug/release and platform.

**system_libdir**  Default = "lib"
    //build/config/linux/pkg_config.gni:44
    CrOS systemroots place pkgconfig files at <systemroot>/usr/share/pkgconfig
    and one of <systemroot>/usr/lib/pkgconfig or <systemroot>/usr/lib64/pkgconfig
    depending on whether the systemroot is for a 32 or 64 bit architecture.
   
    When build under GYP, CrOS board builds specify the 'system_libdir' variable
    as part of the GYP_DEFINES provided by the CrOS emerge build or simple
    chrome build scheme. This variable permits controlling this for GN builds
    in similar fashion by setting the `system_libdir` variable in the build's
    args.gn file to 'lib' or 'lib64' as appropriate for the target architecture.

**target_cpu**  Default = ""
    (Internally set; try `gn help target_cpu`.)

**target_os**  Default = ""
    (Internally set; try `gn help target_os`.)

**target_sysroot**  Default = ""
    //build/config/sysroot.gni:13
    The absolute path of the sysroot that is applied when compiling using
    the target toolchain.

**target_sysroot_dir**  Default = ""
    //build/config/sysroot.gni:16
    The absolute path to directory containing sysroots for linux 32 and 64bit

**toolkit_views**  Default = true
    //build/config/ui.gni:49

**treat_warnings_as_errors**  Default = true
    //build/config/compiler/BUILD.gn:28
    Default to warnings as errors for default workflow, where we catch
    warnings with known toolchains. Allow overriding this e.g. for Chromium
    builds on Linux that could use a different version of the compiler.
    With GCC, warnings in no-Chromium code are always not treated as errors.

**use_afl**  Default = false
    //build/config/sanitizers/sanitizers.gni:74
    Compile for fuzzing with AFL.

**use_allocator**  Default = "none"
    //build/config/allocator.gni:28
    Memory allocator to use. Set to "none" to use default allocator.

**use_alsa**  Default = false
    //media/media_options.gni:69
    Enables runtime selection of ALSA library for audio.

**use_ash**  Default = false
    //build/config/ui.gni:25
    Indicates if Ash is enabled. Ash is the Aura Shell which provides a
    desktop-like environment for Aura. Requires use_aura = true

**use_aura**  Default = false
    //build/config/ui.gni:34
    Indicates if Aura is enabled. Aura is a low-level windowing library, sort
    of a replacement for GDI or GTK.

**use_cfi_cast**  Default = false
    //build/config/sanitizers/sanitizers.gni:63
    Enable checks for bad casts: derived cast and unrelated cast.
    TODO(krasin): remove this, when we're ready to add these checks by default.
    https://crbug.com/626794

**use_cfi_diag**  Default = false
    //build/config/sanitizers/sanitizers.gni:67
    By default, Control Flow Integrity will crash the program if it detects a
    violation. Set this to true to print detailed diagnostics instead.

**use_cras**  Default = false
    //media/media_options.gni:31
    Override to dynamically link the cras (ChromeOS audio) library.

**use_cups**  Default = true
    //printing/features/features.gni:16

**use_custom_libcxx**  Default = false
    //build/config/sanitizers/sanitizers.gni:129

**use_dbus**  Default = false
    //build/config/features.gni:60

**use_debug_fission**  Default = "default"
    //build/config/compiler/compiler.gni:37
    use_debug_fission: whether to use split DWARF debug info
    files. This can reduce link time significantly, but is incompatible
    with some utilities such as icecc and ccache. Requires gold and
    gcc >= 4.8 or clang.
    http://gcc.gnu.org/wiki/DebugFission
   
    This is a placeholder value indicating that the code below should set
    the default.  This is necessary to delay the evaluation of the default
    value expression until after its input values such as use_gold have
    been set, e.g. by a toolchain_args() block.

**use_drfuzz**  Default = false
    //build/config/sanitizers/sanitizers.gni:82
    Compile for fuzzing with Dr. Fuzz
    See http://www.chromium.org/developers/testing/dr-fuzz

**use_evdev_gestures**  Default = false
    //ui/events/ozone/BUILD.gn:12
    Support ChromeOS touchpad gestures with ozone.

**use_experimental_allocator_shim**  Default = false
    //build/config/allocator.gni:33
    TODO(primiano): this should just become the default without having a flag,
    but we need to get there first. http://crbug.com/550886 .
    Causes all the allocations to be routed via allocator_shim.cc.

**use_external_popup_menu**  Default = true
    //build/config/features.gni:69
    Whether or not to use external popup menu.

**use_gconf**  Default = false
    //build/config/features.gni:64
    Option controlling the use of GConf (the classic GNOME configuration
    system).

**use_gio**  Default = false
    //build/config/features.gni:66

**use_glib**  Default = false
    //build/config/ui.gni:37
    Whether we should use glib, a low level C utility library.

**use_gnome_keyring**  Default = false
    //components/os_crypt/features.gni:10
    Whether to use libgnome-keyring (deprecated by libsecret).
    See http://crbug.com/466975 and http://crbug.com/355223.

**use_gold**  Default = false
    //build/config/compiler/compiler.gni:75
    Whether to use the gold linker from binutils instead of lld or bfd.

**use_goma**  Default = false
    //build/toolchain/goma.gni:9
    Set to true to enable distributed compilation using Goma.

**use_incremental_wpo**  Default = false
    //build/config/compiler/compiler.gni:46
    Whether or not we should turn on incremental WPO. Only affects the VS
    Windows build.

**use_kerberos**  Default = true
    //net/features.gni:19
    Enable Kerberos authentication. It is disabled by default on ChromeOS, iOS,
    Chromecast, at least for now. This feature needs configuration (krb5.conf
    and so on).

**use_libfuzzer**  Default = false
    //build/config/sanitizers/sanitizers.gni:71
    Compile for fuzzing with LLVM LibFuzzer.
    See http://www.chromium.org/developers/testing/libfuzzer

**use_libjpeg_turbo**  Default = true
    //third_party/BUILD.gn:17
    Uses libjpeg_turbo as the jpeg implementation. Has no effect if
    use_system_libjpeg is set.

**use_libpci**  Default = false
    //gpu/config/BUILD.gn:11
    Use the PCI lib to collect GPU information on Linux.

**use_lld**  Default = false
    //build/toolchain/toolchain.gni:23
    Set to true to use lld, the LLVM linker. This flag may be used on Windows
    with the shipped LLVM toolchain, or on Linux with a self-built top-of-tree
    LLVM toolchain (see llvm_force_head_revision in
    build/config/compiler/BUILD.gn).

**use_locally_built_instrumented_libraries**  Default = false
    //build/config/sanitizers/sanitizers.gni:46
    Use dynamic libraries instrumented by one of the sanitizers instead of the
    standard system libraries. Set this flag to build the libraries from source.

**use_low_memory_buffer**  Default = false
    //media/media_options.gni:34
    Use low-memory buffers on non-Android builds of Chromecast.

**use_low_quality_image_interpolation**  Default = false
    //third_party/WebKit/Source/config.gni:25
    If true, defaults image interpolation to low quality.

**use_official_google_api_keys**  Default = ""
    //google_apis/BUILD.gn:31
    You can set the variable 'use_official_google_api_keys' to true
    to use the Google-internal file containing official API keys
    for Google Chrome even in a developer build.  Setting this
    variable explicitly to true will cause your build to fail if the
    internal file is missing.
   
    The variable is documented here, but not handled in this file;
    see //google_apis/determine_use_official_keys.gypi for the
    implementation.
   
    Set the variable to false to not use the internal file, even when
    it exists in your checkout.
   
    Leave it unset or set to "" to have the variable
    implicitly set to true if you have
    src/google_apis/internal/google_chrome_api_keys.h in your
    checkout, and implicitly set to false if not.
   
    Note that official builds always behave as if the variable
    was explicitly set to true, i.e. they always use official keys,
    and will fail to build if the internal file is missing.

**use_openh264**  Default = false
    //third_party/openh264/openh264_args.gni:11
    Enable this to build OpenH264 (for encoding, not decoding).
    CHECK THE OPENH264 LICENSE/PATENT BEFORE BUILDING, see
    http://www.openh264.org/.

**use_ozone**  Default = false
    //build/config/ui.gni:30
    Indicates if Ozone is enabled. Ozone is a low-level library layer for Linux
    that does not require X11. Enabling this feature disables use of glib, x11,
    Pango, and Cairo. Default to false on non-Chromecast builds.

**use_partition_alloc**  Default = true
    //base/BUILD.gn:46
    Partition alloc is included by default except iOS.

**use_platform_icu_alternatives**  Default = false
    //url/features.gni:10
    Enables the use of ICU alternatives in lieu of ICU. The flag is used
    for Cronet to reduce the size of the Cronet binary.

**use_prebuilt_instrumented_libraries**  Default = false
    //build/config/sanitizers/sanitizers.gni:42
    Use dynamic libraries instrumented by one of the sanitizers instead of the
    standard system libraries. Set this flag to download prebuilt binaries from
    GCS.

**use_pulseaudio**  Default = false
    //media/media_options.gni:66
    Enables runtime selection of PulseAudio library.

**use_rtti**  Default = false
    //build/config/compiler/BUILD.gn:84
    Build with C++ RTTI enabled. Chromium builds without RTTI by default,
    but some sanitizers are known to require it, like CFI diagnostics
    and UBsan variants.

**use_sanitizer_coverage**  Default = false
    //build/config/sanitizers/sanitizers.gni:134

**use_sysroot**  Default = true
    //build/config/sysroot.gni:18

**use_system_harfbuzz**  Default = false
    //third_party/harfbuzz-ng/BUILD.gn:17
    Blink uses a cutting-edge version of Harfbuzz; most Linux distros do not
    contain a new enough version of the code to work correctly. However,
    ChromeOS chroots (i.e, real ChromeOS builds for devices) do contain a
    new enough version of the library, and so this variable exists so that
    ChromeOS can build against the system lib and keep binary sizes smaller.

**use_system_libjpeg**  Default = false
    //third_party/BUILD.gn:13
    Uses system libjpeg. If true, overrides use_libjpeg_turbo.

**use_system_sqlite**  Default = false
    //third_party/sqlite/BUILD.gn:11
    Controls whether the build should uses the version of sqlite3 library
    shipped with the system (currently only supported on iOS) or the one
    shipped with Chromium source.

**use_system_xcode**  Default = ""
    //build_overrides/build.gni:54
    Use the system install of Xcode for tools like ibtool, libtool, etc.
    This does not affect the compiler. When this variable is false, targets will
    instead use a hermetic install of Xcode. [The hermetic install can be
    obtained with gclient sync after setting the environment variable
    FORCE_MAC_TOOLCHAIN].

**use_thin_lto**  Default = false
    //build/toolchain/toolchain.gni:29
    If used with allow_posix_link_time_opt, it enables the experimental support
    of ThinLTO that links 3x-10x faster but (as of now) does not have all the
    important optimizations such us devirtualization implemented. See also
    http://blog.llvm.org/2016/06/thinlto-scalable-and-incremental-lto.html

**use_udev**  Default = false
    //build/config/features.gni:58
    libudev usage. This currently only affects the content layer.

**use_unofficial_version_number**  Default = true
    //components/version_info/BUILD.gn:10

**use_v4l2_codec**  Default = false
    //media/gpu/args.gni:11
    Indicates if Video4Linux2 codec is used. This is used for all CrOS
    platforms which have v4l2 hardware encoder / decoder.

**use_v4lplugin**  Default = false
    //media/gpu/args.gni:7
    Indicates if V4L plugin is used.

**use_vgem_map**  Default = false
    //ui/ozone/ozone.gni:17
    This enables memory-mapped access to accelerated graphics buffers via the
    VGEM ("virtual GEM") driver. This is currently only available on Chrome OS
    kernels and affects code in the GBM ozone platform.
    TODO(dshwang): remove this flag when all gbm hardware supports vgem map.
    crbug.com/519587

**use_vulcanize**  Default = true
    //chrome/common/features.gni:75
    Use vulcanized HTML/CSS/JS resources to speed up WebUI (chrome://)
    pages. https://github.com/polymer/vulcanize

**use_xcode_clang**  Default = false
    //build/toolchain/toolchain.gni:42
    Compile with Xcode version of clang instead of hermetic version shipped
    with the build. Used on iOS to ship official builds (as they are built
    with the version of clang shipped with Xcode).

**use_xkbcommon**  Default = false
    //ui/base/ui_features.gni:7
    Optional system library.

**v8_android_log_stdout**  Default = false
    //v8/BUILD.gn:21
    Print to stdout on Android.

**v8_can_use_fpu_instructions**  Default = true
    //v8/BUILD.gn:75
    Similar to vfp but on MIPS.

**v8_correctness_fuzzer**  Default = false
    //v8/gni/v8.gni:11
    Includes files needed for correctness fuzzing.

**v8_current_cpu**  Default = "x64"
    //build/config/v8_target_cpu.gni:60
    This argument is declared here so that it can be overridden in toolchains.
    It should never be explicitly set by the user.

**v8_deprecation_warnings**  Default = false
    //v8/BUILD.gn:30
    Enable compiler warnings when using V8_DEPRECATED apis.

**v8_embed_script**  Default = ""
    //v8/BUILD.gn:36
    Embeds the given script into the snapshot.

**v8_enable_backtrace**  Default = ""
    //v8/gni/v8.gni:24
    Support for backtrace_symbols on linux.

**v8_enable_disassembler**  Default = ""
    //v8/BUILD.gn:39
    Sets -dENABLE_DISASSEMBLER.

**v8_enable_gdbjit**  Default = ""
    //v8/BUILD.gn:42
    Sets -dENABLE_GDB_JIT_INTERFACE.

**v8_enable_handle_zapping**  Default = true
    //v8/BUILD.gn:45
    Sets -dENABLE_HANDLE_ZAPPING.

**v8_enable_i18n_support**  Default = true
    //v8/gni/v8.gni:36
    Enable ECMAScript Internationalization API. Enabling this feature will
    add a dependency on the ICU library.

**v8_enable_inspector**  Default = true
    //v8/gni/v8.gni:39
    Enable inspector. See include/v8-inspector.h.

**v8_enable_object_print**  Default = ""
    //v8/BUILD.gn:55
    Sets -dOBJECT_PRINT.

**v8_enable_slow_dchecks**  Default = false
    //v8/BUILD.gn:48
    Enable slow dchecks.

**v8_enable_trace_maps**  Default = ""
    //v8/BUILD.gn:58
    Sets -dTRACE_MAPS.

**v8_enable_v8_checks**  Default = ""
    //v8/BUILD.gn:61
    Sets -dV8_ENABLE_CHECKS.

**v8_enable_verify_heap**  Default = ""
    //v8/BUILD.gn:24
    Sets -DVERIFY_HEAP.

**v8_enable_verify_predictable**  Default = false
    //v8/BUILD.gn:27
    Sets -DVERIFY_PREDICTABLE

**v8_gcmole**  Default = false
    //v8/gni/v8.gni:18
    Indicate if gcmole was fetched as a hook to make it available on swarming.

**v8_has_valgrind**  Default = false
    //v8/gni/v8.gni:15
    Indicate if valgrind was fetched as a custom deps to make it available on
    swarming.

**v8_imminent_deprecation_warnings**  Default = ""
    //v8/BUILD.gn:33
    Enable compiler warnings when using V8_DEPRECATE_SOON apis.

**v8_interpreted_regexp**  Default = false
    //v8/BUILD.gn:52
    Interpreted regexp engine exists as platform-independent alternative
    based where the regular expression is compiled to a bytecode.

**v8_no_inline**  Default = false
    //v8/BUILD.gn:69
    Switches off inlining in V8.

**v8_optimized_debug**  Default = true
    //v8/gni/v8.gni:21
    Turns on compiler optimizations in V8 in Debug build.

**v8_os_page_size**  Default = "0"
    //v8/BUILD.gn:72
    Override OS page size when generating snapshot

**v8_postmortem_support**  Default = false
    //v8/BUILD.gn:66
    With post mortem support enabled, metadata is embedded into libv8 that
    describes various parameters of the VM for use by debuggers. See
    tools/gen-postmortem-metadata.py for details.

**v8_snapshot_toolchain**  Default = ""
    //v8/snapshot_toolchain.gni:34
    The v8 snapshot needs to be built by code that is compiled with a
    toolchain that matches the bit-width of the target CPU, but runs on
    the host.

**v8_target_cpu**  Default = ""
    //build/config/v8_target_cpu.gni:33
    This arg is used when we want to tell the JIT-generating v8 code
    that we want to have it generate for an architecture that is different
    than the architecture that v8 will actually run on; we then run the
    code under an emulator. For example, we might run v8 on x86, but
    generate arm code and run that under emulation.
   
    This arg is defined here rather than in the v8 project because we want
    some of the common architecture-specific args (like arm_float_abi or
    mips_arch_variant) to be set to their defaults either if the current_cpu
    applies *or* if the v8_current_cpu applies.
   
    As described below, you can also specify the v8_target_cpu to use
    indirectly by specifying a `custom_toolchain` that contains v8_$cpu in the
    name after the normal toolchain.
   
    For example, `gn gen --args="custom_toolchain=...:clang_x64_v8_arm64"`
    is equivalent to setting --args=`v8_target_cpu="arm64"`. Setting
    `custom_toolchain` is more verbose but makes the toolchain that is
    (effectively) being used explicit.
   
    v8_target_cpu can only be used to target one architecture in a build,
    so if you wish to build multiple copies of v8 that are targetting
    different architectures, you will need to do something more
    complicated involving multiple toolchains along the lines of
    custom_toolchain, above.

**v8_test_isolation_mode**  Default = "noop"
    //v8/gni/isolate.gni:11
    Sets the test isolation mode (noop|prepare|check).

**v8_use_external_startup_data**  Default = ""
    //v8/gni/v8.gni:32
    Use external files for startup data blobs:
    the JS builtins sources and the start snapshot.

**v8_use_mips_abi_hardfloat**  Default = true
    //v8/BUILD.gn:78
    Similar to the ARM hard float ABI but on MIPS.

**v8_use_snapshot**  Default = true
    //v8/gni/v8.gni:28
    Enable the snapshot feature, for fast context creation.
    http://v8project.blogspot.com/2015/09/custom-startup-snapshots.html

**win_console_app**  Default = false
    //build/config/win/console_app.gni:12
    If true, builds as a console app (rather than a windowed app), which allows
    logging to be printed to the user. This will cause a terminal window to pop
    up when the executable is not run from the command line, so should only be
    used for development. Only has an effect on Windows builds.

