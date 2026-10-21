"""Prepare an additive private candidate-volume profile suffix; no target API.

The caller must still admit its exact original profile transaction, mounted
candidate, guarded execution and restoration. This builder grants none of them.
"""
import hashlib
import re

ORIGINAL_PROFILE_SHA256 = '894aa5a61a2bf6267b461c6774a0bdc913a96e7f581ea7e4e3762c4d71709a30'
VOLUME_PATH = re.compile(r'/private/tmp/pt24g-editor-capture-volume-[a-z0-9][a-z0-9-]{0,47}')


def profile_append(original, directory):
    """Use the paired directory-mount syntax of the installed exact profile.

    The existing Dev mount is retained. DH3/PTCapture and uaehf2 are reserved
    only for this private prospective case, never a default configuration.
    """
    if (not isinstance(original, bytes) or len(original) != 4828 or
            hashlib.sha256(original).hexdigest() != ORIGINAL_PROFILE_SHA256):
        raise ValueError('Exact original shared profile bytes required')
    if not isinstance(directory, str) or VOLUME_PATH.fullmatch(directory) is None:
        raise ValueError('Fresh bounded local candidate-volume path required')
    if (not original.endswith(b'\n') or b'absolute_mouse=' in original or
            b'PTCapture' in original or b'DH3:' in original or b'uaehf2=' in original):
        raise ValueError('Prospective profile slots are already occupied')
    mount = ('DH3:PTCapture:' + directory + ',-128\n').encode('ascii')
    return b'absolute_mouse=mousehack\nfilesystem2=rw,' + mount + b'uaehf2=dir,rw,' + mount
