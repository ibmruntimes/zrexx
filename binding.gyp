{
    "variables": {
        "NODE_VERSION%":"<!(node -p \"process.versions.node.split(\\\".\\\")[0]\")"
    },
    "targets": [{
        "target_name": "zrexx",
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
          [ "NODE_VERSION < 16", {
            "cflags": [  "-qascii" ],
          }],
        ],

        "libraries": [],
        "dependencies": [
            "<!(node -p \"require('node-addon-api').gyp\")"
        ],
        "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ]
    }]
}
