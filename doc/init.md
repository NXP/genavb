Initialization API usage {#init_usage}
=======================

\if LINUX

The initialization API's allow to initialize the GenAVB/TSN stack library. To initialize the library call ::genavb_init, to exit ::genavb_exit. The handle returned by ::genavb_init is
required to call most other API's.

\else

The initialization API's allow to initialize the GenAVB/TSN stack. To initialize the stack call ::genavb_init, to exit ::genavb_exit. The handle returned by ::genavb_init is
required to call most other API's.

It's possible to change the default stack configuration, before calling ::genavb_init. To retrieve the default configuration use ::genavb_get_default_config (with a genavb_config structure allocated in the stack or heap). Then, override the required settings in genavb_config and call ::genavb_set_config. After calling ::genavb_init, genavb_config structure may be discarded.

\endif
