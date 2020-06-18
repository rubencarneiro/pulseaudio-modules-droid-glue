/*
 * Copyright (C) 2015 Jolla Ltd.
 *
 * Contact: Juho Hämäläinen <juho.hamalainen@jolla.com>
 *
 * These PulseAudio Modules are free software; you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <dlfcn.h>

#ifdef HAVE_VALGRIND_MEMCHECK_H
#include <valgrind/memcheck.h>
#endif

#include <pulse/rtclock.h>
#include <pulse/timeval.h>
#include <pulse/xmalloc.h>

#include <pulsecore/core.h>
#include <pulsecore/i18n.h>
#include <pulsecore/module.h>
#include <pulsecore/modargs.h>
#include <pulsecore/log.h>
#include <pulsecore/macro.h>

/*
 * Because the name of the droid module can change according to the module
 * suffix, use the forwarding header which will have the correct name filled
 * by autoconf.
 */
#include "droid-util.h"

#include "module-symdef-compat.h"

#include <hybris/common/binding.h>
#include <audioflingerglue.h>

PA_MODULE_AUTHOR("Juho Hämäläinen");
PA_MODULE_DESCRIPTION("Droid AudioFlinger Glue");
PA_MODULE_VERSION(PACKAGE_VERSION);
PA_MODULE_USAGE(
        "module_id=<which droid hw module to load, default primary> "
        "lib=<absolute path to audioflingerglue library. if not defined try to autodetect>"
);

static const char* const valid_modargs[] = {
    "module_id",
    "lib",
    NULL,
};

#if defined(__LP64__)
# define AF_LIB LIB_AF_BASE_PATH "/lib64/" LIB_AF_NAME
#else
# define AF_LIB LIB_AF_BASE_PATH "/lib/" LIB_AF_NAME
#endif

#define DEFAULT_MODULE_ID "primary"

static void *audioflingerglue_handle;

struct userdata {
    pa_core *core;
    pa_module *module;

    pa_droid_hw_module *hw_module;
    DroidAfGlue *glue;
};

static int set_parameters_cb(const char *key_value_pairs, void *userdata) {
    struct userdata *u;
    int ret;

    pa_assert_se((u = userdata));

    pa_log_debug("Glue set_parameters(\"%s\")", key_value_pairs);

    pa_droid_hw_module_lock(u->hw_module);
    ret = u->hw_module->device->set_parameters(u->hw_module->device, key_value_pairs);
    pa_droid_hw_module_unlock(u->hw_module);

    if (ret != 0)
        pa_log_warn("Glue set_parameters(\"%s\") failed: %d", key_value_pairs, ret);

    return ret;
}

static int get_parameters_cb(const char *keys, char **reply, void *userdata) {
    struct userdata *u;

    pa_assert_se((u = userdata));

    pa_droid_hw_module_lock(u->hw_module);
    *reply = u->hw_module->device->get_parameters(u->hw_module->device, keys);
    pa_droid_hw_module_unlock(u->hw_module);

    pa_log_debug("Glue get_parameters(\"%s\"): \"%s\"", keys, *reply ? *reply : "<null>");

    return *reply ? 0 : 1;
}

static bool audioflingerglue_initialize(const char *path) {
    if ((audioflingerglue_handle = android_dlopen(path, RTLD_LAZY)))
        return true;

    return false;
}

static bool file_exists(const char *path) {
    return access(path, F_OK) == 0 ? true : false;
}

int pa__init(pa_module *m) {
    pa_modargs *ma = NULL;
    const char *module_id;
    const char *lib_path;
    DroidAfGlueCallbacks cb;

    pa_assert(m);

    if (!(ma = pa_modargs_new(m->argument, valid_modargs))) {
        pa_log("Failed to parse module arguments.");
        goto fail;
    }

    if ((lib_path = pa_modargs_get_value(ma, "lib", NULL))) {
        if (!file_exists(lib_path)) {
            pa_log("Audioflingerglue library with path '%s' not found.", lib_path);
            goto fail;
        }
    } else
        lib_path = AF_LIB;

    if (!lib_path) {
        pa_log("Could not find audioflingerglue library.");
        goto fail;
    }

    if (!audioflingerglue_initialize(lib_path)) {
        pa_log("Could not load audioflingerglue library.");
        goto fail;
    }

    struct userdata *u = pa_xnew0(struct userdata, 1);
    u->core = m->core;
    m->userdata = u;

    module_id = pa_modargs_get_value(ma, "module_id", DEFAULT_MODULE_ID);

    if (!(u->hw_module = pa_droid_hw_module_get(u->core, NULL, module_id))) {
        pa_log("Couldn't get hw module %s, is module-droid-card loaded?", module_id);
        goto fail;
    }

    cb.set_parameters = set_parameters_cb;
    cb.get_parameters = get_parameters_cb;

    u->glue = droid_afglue_connect(&cb, u);

    if (!u->glue) {
        pa_log("Couldn't establish connection to miniafservice.");
        goto fail;
    }

    return 0;

fail:
    if (ma)
        pa_modargs_free(ma);

    pa__done(m);

    return -1;
}

void pa__done(pa_module *m) {
    struct userdata *u;

    pa_assert(m);

    if ((u = m->userdata)) {

        if (u->glue)
            droid_afglue_disconnect(u->glue);

        if (u->hw_module)
            pa_droid_hw_module_unref(u->hw_module);

        pa_xfree(u);
    }
}
