{
    "targets": [{
        "target_name": "zrexx",
        "cflags!": [ "-fno-exceptions"],
        "cflags": [  "-qascii" ],
        "cflags_cc!": ["-fno-exceptions" ],
        "cflags_cc": ["-qascii" ],
        "include_dirs": [
            "<!@(node -p \"require('node-addon-api').include\")"
        ],
        "conditions": [
          [ "OS==\"zos\"", {
            "sources": [
               "cppsrc/zrexx.cc",
               "cppsrc/zrex_impl.cc"
            ],
          }],
        ],

        "libraries": [],
        "dependencies": [
            "<!(node -p \"require('node-addon-api').gyp\")"
        ],
        "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS", "_AE_BIMODAL=1", "_ALL_SOURCE", "_ENHANCED_ASCII_EXT=0x42020010", "_LARGE_TIME_API", "_OPEN_MSGQ_EXT", "_OPEN_SYS_FILE_EXT=1", "_OPEN_SYS_SOCK_IPV6", "_UNIX03_SOURCE", "_UNIX03_THREADS", "_UNIX03_WITHDRAWN", "_XOPEN_SOURCE=600", "_XOPEN_SOURCE_EXTENDED", 
        ]
    }]
}
