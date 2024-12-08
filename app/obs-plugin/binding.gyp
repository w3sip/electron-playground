{
  "targets": [
    {
      "target_name": "obsplugin",
      "sources": [ "obs_plugin.cpp" ],
      "cflags_cc": [
        "-std=c++20"
      ],
      "include_dirs": [ "<(module_root_dir)/../../obs-studio/libobs" ],
      "libraries" : [ "-Wl,-rpath,<(module_root_dir)/../Frameworks",
                     "-F<(module_root_dir)/../Frameworks",
                    "-framework libobs" ]
    }
  ]
}
