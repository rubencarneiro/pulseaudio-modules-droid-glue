AM_LIBADD = \
	$(PULSEAUDIO_LIBS) \
	$(HYBRIS_LIBS) \
	$(DROIDUTIL_LIBS)
AM_CFLAGS = \
	$(DROID_DEVICE_CFLAGS) \
	$(PULSEAUDIO_CFLAGS) \
	$(DROIDHEADERS_CFLAGS) \
	$(HYBRIS_CFLAGS) \
	$(LIB_AF_CFLAGS) \
	$(DROIDUTIL_CFLAGS) \
	-DPULSEAUDIO_VERSION=@PA_MAJOR@ \
	-I$(top_srcdir)/src/glue


modlibexec_LTLIBRARIES = module-droid-glue$(DROID_MODULE_SUFFIX).la

noinst_HEADERS = module-symdef-compat.h

module_droid_glue__DROID_MODULE_SUFFIX__la_SOURCES = /usr/share/audioflingerglue/hybris.c module-droid-glue.c
module_droid_glue__DROID_MODULE_SUFFIX__la_LDFLAGS = -module -avoid-version -Wl,-no-undefined -Wl,-z,noexecstack -lhybris-common
module_droid_glue__DROID_MODULE_SUFFIX__la_LIBADD = $(AM_LIBADD) -lm
module_droid_glue__DROID_MODULE_SUFFIX__la_CFLAGS = $(AM_CFLAGS) -I/usr/include/audioflingerglue -DPA_MODULE_NAME=module_droid_glue$(subst -,_,$(DROID_MODULE_SUFFIX))
