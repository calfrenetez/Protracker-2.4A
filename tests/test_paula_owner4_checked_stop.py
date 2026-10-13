"""Fixed-four constructed owner checked-stop serialized HOST qualifier.
37 strict units and 90 pinned Source bodies; 21 genuine HOST cases.
15 restored paths and 6 deliberate reachable HOLD paths.
Native/frontend/emulator/physical admission remains disabled.
Canonical runtime qualification is separate from the private HOST result."""
from pathlib import Path, PurePosixPath
import hashlib
import json
import os
import posixpath
import re
import stat
import subprocess
import tempfile
import unittest

NATIVE_ADMITTED = False
FRONTEND_ADMITTED = False
ROOT = Path(__file__).resolve().parents[1]
PRESERVATION_ROOT = ROOT
FIXTURE_PINS = {'src/core/amigus_reservation.h': {'bytes': 3404, 'sha256': 'ee57be55b9865fd19d14b27b22af6e2aa6b0466b7f21da5b313016efe9a0b646'}, 'src/core/amigus_sample_ram.h': {'bytes': 2813, 'sha256': '005111323eb1884112230ea415fbfb75e01d560618326148e08790d4f23fadf6'}, 'src/core/amigus_wavetable_cache.h': {'bytes': 3792, 'sha256': '8c449f009eac14807a9521d16a756345dd531e47b50312a3cabf96ee7ea2ee9c'}, 'src/core/channels.c': {'bytes': 6996, 'sha256': 'b6316f4a19e737d3f5a19bd04f22f43b0da72188d2a0623404dc4a27bc9c622d'}, 'src/core/channels.h': {'bytes': 1818, 'sha256': '9eadeee6befb0f291e62c654a8ccd5ed6c6e7030f8c8c93587566fb06eb99517'}, 'src/core/document.c': {'bytes': 9127, 'sha256': 'be88c9a4ad7aa31cc6df7446673c0bed972776918b32f31ef2dc6260528b196a'}, 'src/core/document.h': {'bytes': 1965, 'sha256': 'f66757baf8d762bf0413afd2b4e7b2e589ad6f6a6e2b59f6f72d9458a3fa279f'}, 'src/core/elapsed_clock.c': {'bytes': 3917, 'sha256': '7db37ec7184235f0776ba773996271641b0d0a8a0efcde3abb1978ab6b414ea8'}, 'src/core/elapsed_clock.h': {'bytes': 1659, 'sha256': 'd5db62de2b938228edb95b1f3efb520d58c5e37a7f10f0b6cca1fc5d8e9ec37e'}, 'src/core/flow.c': {'bytes': 8904, 'sha256': 'c1850d6202a01805876ce6afa903ac4ae145757410e7597d70f2601903e79783'}, 'src/core/flow.h': {'bytes': 3803, 'sha256': 'db5a78efb5344d95f145d70a6c26c664a832410b8ea741c56f0de831d4163046'}, 'src/core/frame_clock.c': {'bytes': 1209, 'sha256': 'e5d9c6c9dc7ea48daf6d099dc5ade8ec2260d8009f40eebd26fa32bd06490a22'}, 'src/core/frame_clock.h': {'bytes': 884, 'sha256': '77bb574a681a492967a6dc938e5eab7e5f28c70f73fd18189de6ed379ccbc54c'}, 'src/core/mod_inspect.c': {'bytes': 3513, 'sha256': '278f601c17ad72a095e6eb17c4f5bbe321bf342edebacf7598ca830f2743b511'}, 'src/core/mod_inspect.h': {'bytes': 1240, 'sha256': '9b80158f6e9db0ccde95ba16f58a6ed4292ebae056e500796b074842abf8a29f'}, 'src/core/mod_project.c': {'bytes': 23640, 'sha256': '793c9ece248b586dd75e7b3ffefefe446bfbc6d4cc476fc2a61a4a1ce8877b09'}, 'src/core/mod_project.h': {'bytes': 5051, 'sha256': 'c28407161729fc0fea2398f79341a866f03913b092769674bd871bb23adb59ee'}, 'src/core/pattern.c': {'bytes': 15753, 'sha256': '0d22aff4b63fae85fa44f063ba6c2eeba37467843e4fcceb493037036366431e'}, 'src/core/pattern.h': {'bytes': 3642, 'sha256': 'f8c9abde09c4fe495ae77f25c070da647eab038845729edc83adec08c5678d53'}, 'src/core/paula_render_voice.c': {'bytes': 1997, 'sha256': 'c037e6d3e59d6c898e78ebba05036d960ced3a2758a08cf36adbdcd4780ecc82'}, 'src/core/paula_render_voice.h': {'bytes': 1642, 'sha256': '11126de4c05368ab1acdcfe1f92b56df03b8233f7e2ec0c304c9d5922e913c77'}, 'src/core/paula_segment_boundary.c': {'bytes': 5336, 'sha256': '3f639f4e7a9936d420f7f6fe79c592bb44bf29a279605500592e7b104b0872d6'}, 'src/core/paula_segment_boundary.h': {'bytes': 3210, 'sha256': '9dc4d6e92812885cde609f7b3f9fe9aacb0d19110f592d7e3ccfa22ce779a2f5'}, 'src/core/paula_segment_repeat_plan.c': {'bytes': 8911, 'sha256': '06ca2e445a951d14960d4f9cd64efd5041ffb451a7829fcb1cfa7cf3efddd576'}, 'src/core/paula_segment_repeat_plan.h': {'bytes': 6230, 'sha256': '79c93ad567cbe0f811023c7af751a3cd01405108d3b7a6c81ef3f50df3ef218d'}, 'src/core/pcm.c': {'bytes': 7061, 'sha256': 'ddb208ff5dd445c73d1b573eac1aed3e74bc654fefc2662627d4c0d760f99266'}, 'src/core/pcm.h': {'bytes': 2494, 'sha256': '9b6996caf7c81bd53034f53e008e7681885ede2711dddf7c2027c3bab06cb10e'}, 'src/core/pcm_filtered.c': {'bytes': 4604, 'sha256': '198e436dd49fbc3984cc612554961bd70debad6b8ba4b4b9b4daa8020715a5c9'}, 'src/core/pcm_internal.h': {'bytes': 615, 'sha256': '1abbf89dfc2b640624d3f2cb16c2c715d818deb4a88f5284383d386a35c0188b'}, 'src/core/pitch.c': {'bytes': 6193, 'sha256': '9088859aa6a2812510b3a28334420030add960afa5f2cca3b41946018f0c04d5'}, 'src/core/pitch.h': {'bytes': 794, 'sha256': '7ec69432c14acb87bc3ba357fe1031023942ebe4ce35e2193d46c16f7068f8b4'}, 'src/core/pitch_tables.h': {'bytes': 2787, 'sha256': '436cb7721c10ef2ef010d7fa6c2b0e46ccec2a4dfc2f7ce9af76a2bb0eb7812a'}, 'src/core/playback_internal.h': {'bytes': 1893, 'sha256': '5d76f20f2c9a7eda89c3543a897f053c97f9460a847d2d70b32b8b71c14775e2'}, 'src/core/playback_pcm.c': {'bytes': 11344, 'sha256': 'f00995c1ca81176fc244968b4276b41624767f42c2963eb90482f23953d09a1f'}, 'src/core/playback_pcm.h': {'bytes': 6034, 'sha256': 'b5056427354dc20c166cd6da424b211b206835840555bbd8423b2858bc7d0e65'}, 'src/core/pp20.c': {'bytes': 7187, 'sha256': '3f03dac4f224f54a72437370d94e1cd09ce9222a3748a0e805181ae26c5f1465'}, 'src/core/pp20.h': {'bytes': 966, 'sha256': 'd62562e79452f53a0dc9cdc753e7f337d0e305505408199e805f8a43f862dbf6'}, 'src/core/project.c': {'bytes': 47003, 'sha256': 'f1bad97dc044e3976936be95c4e42051e8b7a79aab643a1c984a14a1dd24c75a'}, 'src/core/project.h': {'bytes': 8827, 'sha256': 'e1a749800258b1f1db1dedcd7d5d36970abbf7494279cb3669212757f982e21a'}, 'src/core/raw.c': {'bytes': 3920, 'sha256': '460fe21a893b38f14e4af797022f6409504b125f562509b59efef713dc704377'}, 'src/core/raw.h': {'bytes': 1171, 'sha256': 'f9f47added4df41bb7b15aa3b1b7a4d8131a2c4aa19266436318483a41bc0f3b'}, 'src/core/render.c': {'bytes': 63608, 'sha256': 'acc53af54275d5ec481ca2128dcf72fde305bf91ed28295fd781a81f42966936'}, 'src/core/render.h': {'bytes': 12179, 'sha256': 'aa531e26bcf3d3fbe6ddfb61a3ea86ae2c1aeb90016e41c4649076a33793bc2f'}, 'src/core/render_commands.h': {'bytes': 9193, 'sha256': 'ea176e8382ea808929d11571396d6214bb40d2fce8defc9b19569c1bb5c30d14'}, 'src/core/render_lookahead.h': {'bytes': 1954, 'sha256': '6ecdad4402eb721aad8d3b55d6d8fc87a9f8135962c8638f0f1919527c4fbf03'}, 'src/core/render_storage_internal.h': {'bytes': 662, 'sha256': 'ca1eb7fea1a37cba6ebfedd2e91f6e31a0c6d3f02feee383d184e982fad3953e'}, 'src/core/sample_cache.c': {'bytes': 4018, 'sha256': 'ebe0d55d650ae0bc0aed9f5a5628dca942a6914082c25366dfe52dcdddbbf39a'}, 'src/core/sample_cache.h': {'bytes': 1999, 'sha256': 'df19898830aa7696c38898feae15e3b437679af23d5ea3b69e8aa923e927e5c6'}, 'src/core/sample_usage.c': {'bytes': 8253, 'sha256': '14520084e266041ea257bc7e1b5145f9322c5549e4e4377f5b962232f089b160'}, 'src/core/sample_usage.h': {'bytes': 3167, 'sha256': '0ed540356b9ff1bd2ae651de2ee183b6c0c45af84f352eb02fab07d9155565a0'}, 'src/core/scheduled_output.h': {'bytes': 6515, 'sha256': '3289945d9446b373d8aa1897cb2c432f08ef253b443a81cd4dde39230df9b9da'}, 'src/core/scheduled_readers.c': {'bytes': 28153, 'sha256': 'd0b038022fc5d6b76c589af3094bda338374b2e151143a81a071f803e1e879f0'}, 'src/core/scheduled_readers.h': {'bytes': 9906, 'sha256': '5a63c3da2c4b08eb9329d33c3679776c8554f6f96bb56bc3590f4afd9fd5e52c'}, 'src/core/sinc_kernel.h': {'bytes': 28756, 'sha256': '058629396a0b22daa67f75c909adf9181fc4e52bfc668890fe2abc3231c3a185'}, 'src/core/slices.c': {'bytes': 6060, 'sha256': '2951045844f6fdebdd80afe12fe7a5afc1f37bf837c2f49d840f14867835342f'}, 'src/core/slices.h': {'bytes': 2003, 'sha256': '31cd82785c2b86c41d9cde1f7a29056c9d70ec41fb7a9bfb1a5679ba762c24f7'}, 'src/core/svx.c': {'bytes': 7817, 'sha256': '4712a8d846dd8e5a36fbbdb9b2db7c2fc71ed215be6b114da7d0285cebf69813'}, 'src/core/svx.h': {'bytes': 1442, 'sha256': '824541cd25b72b5820e091cfc9f4e6a5f41351fd0630dddc09aefc0405036814'}, 'src/core/timeline.c': {'bytes': 1598, 'sha256': '2296df1f1d13336bacb40cd867268dbd1c52d1ed0353256f5022da536516effd'}, 'src/core/timeline.h': {'bytes': 1150, 'sha256': 'ef1640b5c7dd9e48e1e13a9963c21c53b913d062b172a988e91abb7532defa64'}, 'src/core/voice.c': {'bytes': 11427, 'sha256': '9da52c6a82f9ea753655ed4d196946198316cb5327078ffbf354a6dc66e48c68'}, 'src/core/voice.h': {'bytes': 4184, 'sha256': '386b10545fb8ae615b5a45718d90be60b0999b2d63fffd835b6be540d7dca479'}, 'src/core/voice_internal.h': {'bytes': 934, 'sha256': '02f7f0af75f1a60a7907fbdcbfa754f3d2a06e9073d2bb5e4888f36a66ce5973'}, 'src/core/wav.c': {'bytes': 7200, 'sha256': '499264a12362a271c20dcc13f4e5e38a470aa5a4ddce90a44c5193f4d8be916f'}, 'src/core/wav.h': {'bytes': 1384, 'sha256': '8dd0f4454304babb53eef5ecb97cde7bfe6569d292cc0e9a0f204c4a587f2028'}, 'src/editor/paula_owner4.c': {'bytes': 17696, 'sha256': '016c4aedccbc6470e9e830b8822051c1b7f52574ba834ec242f359118e555eb2'}, 'src/editor/paula_owner4.h': {'bytes': 3524, 'sha256': 'f6fbcb793eea531abd877e51c99e02ff6be696bd1292b596761cd65429f1e108'}, 'src/editor/paula_segment_future.c': {'bytes': 32911, 'sha256': 'f62e8f25b9e92f5d809958976e99db5f9686521f14344bba1c82d280c736ab71'}, 'src/editor/paula_segment_future.h': {'bytes': 4994, 'sha256': '5a0927d1c787151e58083b1d19f1f7d884293f8e88a5c1e6e41a18712ee57897'}, 'src/editor/paula_segment_sequence.c': {'bytes': 31252, 'sha256': 'c1920acc24d3ac4661d6dc6e4eaa6010ccd0f60b5c5b41aec3235ec8b100d1b1'}, 'src/editor/paula_segment_sequence.h': {'bytes': 6254, 'sha256': 'c5206980afc39ee46964899b10fe56e3c4dba9ddc2a96a4416e6b1efc10672fe'}, 'src/editor/project_snapshot.h': {'bytes': 1203, 'sha256': 'dbb1206c1e8bfd317b515086f9d064c6eacd2450228893bb394f278ed6c0b2df'}, 'src/editor/sampler.c': {'bytes': 82424, 'sha256': 'd3bba34d4725a6f9cc311fdf3ba2d2233ed84f9ba43d57dd1bc92226446bdc57'}, 'src/editor/sampler.h': {'bytes': 6531, 'sha256': 'c641ee916de9b292c529bf558ce34ea96d3449d4255510a0122cf67e5822fae9'}, 'src/editor/sampler_internal.h': {'bytes': 4851, 'sha256': '557dc012f010b66ee8dc2ff87b2fe1d906b3d7d9fbb6685134ae87302dd00f1f'}, 'src/editor/sampler_lifetime.c': {'bytes': 8105, 'sha256': '5a76953c7cae9e246f608883b91709f87cd9996135237057795dba0fc4a15ac1'}, 'src/editor/sampler_lifetime_internal.h': {'bytes': 2463, 'sha256': '3e93926831670f4f313b9f8fec08da3e6cd70b494de4d0e6610d0f681945cdcf'}, 'src/editor/sampler_master_guard_internal.h': {'bytes': 1059, 'sha256': 'f691504b22dd9250a3442953527470e4dd813a4d8eea900412b2afe1d3ab9a7a'}, 'src/editor/sampler_paula.c': {'bytes': 7401, 'sha256': 'bbab9cdb221b03d3eb402d615aba0f54a4d4e4a5e8b637c6856b66d04ea3f03e'}, 'src/editor/sampler_paula.h': {'bytes': 2718, 'sha256': '3f25fa64f67c4f6115cbbefd109e6d17e8f648fe9ba955917a6e05f6158a4890'}, 'src/editor/sampler_paula_internal.h': {'bytes': 3013, 'sha256': 'fdef77b3f4f00b9c89b4ec05f3915e0334d4aa340673b0dc640eaadef8cf75eb'}, 'src/editor/sampler_paula_readers.c': {'bytes': 50873, 'sha256': '90d2996276422d8bd1eb090fb4442e9580f3aff482a95715a10dac3f2e4b35d7'}, 'src/editor/sampler_paula_readers.h': {'bytes': 9761, 'sha256': 'c59fb4022270b6880ef3a430859ee064436f6f1bf4ba3674f546e958ceddc85c'}, 'src/editor/sampler_paula_readers_inventory_internal.h': {'bytes': 1179, 'sha256': 'e57a556e6d33a267db056ed4ab0bb6ce8b1b9a1584dd276e27745fac4dbb9695'}, 'src/editor/sampler_paula_segment_readers.c': {'bytes': 39911, 'sha256': '0b1426e14c98231cf5554c275305121d542b0940cc3a0e4c3209225312c9114d'}, 'src/editor/sampler_paula_segment_readers.h': {'bytes': 6700, 'sha256': '0783ffb4597286e162426d86832c0777b30df0944fa14594c28c57adf8e3a837'}, 'src/editor/sampler_workflow.h': {'bytes': 4356, 'sha256': 'a1eff0addfd1fded1185029f68bd9d6d885d5a421b702b40f76cb28af63fac1a'}, 'src/editor/slots.c': {'bytes': 4237, 'sha256': 'ac9e10cdf165688b98d4a5562966fb7eee8e8a52376de065e3c61251f57045f8'}, 'tests/owner4_future60_donor.c': {'bytes': 46446, 'sha256': '9ca96c4e92bbfb05aba93f976581716fda4ff2d1dfa4148ee5f6c18a39ceff9e'}, 'tests/paula_owner4_genuine_owner_proposal.c': {'bytes': 19789, 'sha256': '45f2514daadc9253b54ad0a99477b04a24aef5232bc71ceb03e410b7b258fce3'}, 'tests/paula_segment_repeat_readers_test.c': {'bytes': 37450, 'sha256': '66bb1deed3d2f56393eccf6f6657289d29fa26f899de16e64c94dd58b749998e'}}

def source_path(relative):
    return ROOT / relative
SOURCES = ('tests/paula_owner4_genuine_owner_proposal.c', 'src/core/paula_segment_repeat_plan.c', 'src/core/paula_render_voice.c', 'src/core/voice.c', 'src/editor/sampler_paula.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c', 'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/project.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/channels.c', 'src/core/pcm.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/editor/sampler_paula_segment_readers.c', 'src/core/scheduled_readers.c', 'src/core/elapsed_clock.c', 'src/editor/sampler_paula_readers.c', 'src/core/paula_segment_boundary.c', 'src/core/render.c', 'src/core/pitch.c', 'src/core/timeline.c', 'src/core/frame_clock.c', 'src/core/flow.c', 'src/editor/paula_segment_sequence.c', 'src/editor/paula_segment_future.c', 'src/editor/paula_owner4.c', 'src/editor/sampler_lifetime.c', 'src/core/sample_usage.c')
CFLAGS = ('-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG', '-fsanitize=address,undefined', '-Isrc/core', '-Itests', '-I.')
ORACLE = 'paula owner4 proposal cases=21 restored=15 deliberate_HOLD=6\n'
PROTECTED = ('AGENTS.md', 'amiga-test.json', 'src/editor/editor.c', 'src/editor/editor.h', 'src/editor/view.c', 'src/editor/view.h', 'src/native/editor_main.c', 'src/native/pattern_display.c', 'src/native/pattern_display.h', 'tests/editor_test.c', 'tests/native_prepared_test.c', 'tests/test_editor.py', 'tools/build_core_tests.py', 'tools/test_sampler_emulator.py', 'tools/shared_infra_sampler.py', 'tools/shared_infra_ui.py')
RAW_FONT_SHA256 = '56be43ae731975ee8530eda153bd9b1b44329d6af4992b2436d3938d38cb9091'
FONT_HEADER_SHA256 = '0c77c7d479f4afa90bd053e7fc1390ac8397e14faa976aa76297065a645778d0'

def metadata_pin(path):
    actual = path.lstat()
    assert stat.S_ISREG(actual.st_mode), "ordinary regular input required"
    return {name: getattr(actual, name, 0) for name in (
        "st_dev", "st_ino", "st_mode", "st_nlink", "st_size", "st_uid", "st_gid",
        "st_mtime_ns", "st_ctime_ns", "st_flags")}


def ordinary_pin(path):
    before = metadata_pin(path)
    assert not (before["st_flags"] & 0x40000000), "dataless body read refused"
    descriptor = os.open(path, os.O_RDONLY | os.O_NOFOLLOW)
    try:
        opened = os.fstat(descriptor)
        assert {name: getattr(opened, name, 0) for name in before} == before
        body = bytearray()
        while True:
            chunk = os.read(descriptor, 65536)
            if not chunk:
                break
            body.extend(chunk)
        after = os.fstat(descriptor)
        assert {name: getattr(after, name, 0) for name in before} == before
    finally:
        os.close(descriptor)
    assert metadata_pin(path) == before and len(body) == before["st_size"]
    return {"bytes": len(body), "sha256": hashlib.sha256(body).hexdigest(),
            "metadata": before}, bytes(body)


def preservation_snapshot():
    observed = {}
    for relative in PROTECTED:
        path = PRESERVATION_ROOT / relative
        try:
            path.lstat()
        except FileNotFoundError:
            observed[relative] = {"state": "ABSENT"}
        else:
            fact, _ = ordinary_pin(path)
            observed[relative] = {"state": "PRESENT", "pin": fact}
    for relative in ('src/editor/sampler_paula_readers.c', 'src/editor/sampler_paula_readers.h', 'src/editor/paula_readers_song.c', 'src/editor/paula_readers_song.h'):
        fact, _ = ordinary_pin(PRESERVATION_ROOT / relative)
        observed[relative] = {"state": "PRESENT", "pin": fact}
    lock_pin, lock_body = ordinary_pin(PRESERVATION_ROOT / "baseline.lock.json")
    observed["baseline.lock.json"] = lock_pin
    baseline = json.loads(lock_body)
    assert baseline["upstream_commit"] == "4e9df4ada0e55869477e1e45d00227719010e0ea"
    assert len(baseline["files"]) == 51
    for entry in baseline["files"]:
        relative = PurePosixPath(entry["path"])
        assert not relative.is_absolute() and ".." not in relative.parts
        path = PRESERVATION_ROOT / "vendor/pt23f" / relative
        metadata = metadata_pin(path)
        if metadata["st_flags"] & 0x40000000:
            # Unselected placeholders stay metadata-only. No hydration, body
            # hash or current-byte equivalence is inferred from the manifest.
            observed["vendor/pt23f/" + str(relative)] = {
                "state": "DATALESS_METADATA_ONLY", "metadata": metadata,
                "historical_manifest_sha256": entry["sha256"],
                "actual_body_verification": "NOT_PERFORMED"}
        else:
            fact, _ = ordinary_pin(path)
            assert fact["sha256"] == entry["sha256"], "classic baseline changed"
            observed["vendor/pt23f/" + str(relative)] = fact
    return observed


def source_snapshot(private):
    """Copy only the exact units and their literal project include closure."""
    includes = re.compile(rb'^\s*#\s*include\s*"([^"\r\n]+)"', re.MULTILINE)
    pending = list(reversed(SOURCES))
    observed = {}
    while pending:
        relative = pending.pop()
        if relative in observed:
            continue
        path = PurePosixPath(relative)
        assert not path.is_absolute() and ".." not in path.parts
        assert path.parts[0] in ("src", "tests"), "project source only"
        fact, body = ordinary_pin(source_path(relative))
        if relative in FIXTURE_PINS:
            assert fact["bytes"] == FIXTURE_PINS[relative]["bytes"]
            assert fact["sha256"] == FIXTURE_PINS[relative]["sha256"]
        observed[relative] = fact
        copied = private / relative
        copied.parent.mkdir(parents=True, exist_ok=True)
        with copied.open("xb") as stream:
            assert stream.write(body) == len(body)
        copied.chmod(0o400)
        assert ordinary_pin(copied)[0]["sha256"] == fact["sha256"]
        for included in includes.findall(body):
            name = included.decode("ascii")
            if name == "pt_font.h":
                continue  # The independently checked deterministic header.
            assert not PurePosixPath(name).is_absolute()
            selected = None
            for directory in (str(path.parent), "src/core", "tests", "."):
                candidate = posixpath.normpath(posixpath.join(directory, name))
                parts = PurePosixPath(candidate).parts
                if not parts or parts[0] not in ("src", "tests") or ".." in parts:
                    continue
                try:
                    source_path(candidate).lstat()
                except FileNotFoundError:
                    continue
                selected = candidate
                break
            assert selected is not None, "unresolved project include: " + name
            pending.append(selected)
    return observed


def verify_sources(observed):
    for relative, fact in observed.items():
        assert ordinary_pin(source_path(relative))[0] == fact, "source changed: " + relative


class PaulaOwner4CheckedStopHost(unittest.TestCase):
    def test_genuine_constructed_owner_checked_stop(self):
        self.assertIsNotNone(ORACLE, "frozen complete fixture oracle required")
        self.assertEqual(len(SOURCES), 37)
        self.assertEqual(len(set(SOURCES)), 37)
        before = preservation_snapshot()
        source_pins = {}
        try:
            font_pin, raw_font = ordinary_pin(PRESERVATION_ROOT / "vendor/pt23f/raw/ptfont.raw")
            self.assertEqual(font_pin["bytes"], 580)
            self.assertEqual(font_pin["sha256"], RAW_FONT_SHA256)
            font_header = ("static const unsigned char pt_font[580] = {"
                           + ",".join(str(value) for value in raw_font)
                           + "};\n").encode("ascii")
            self.assertEqual(len(font_header), 1630)
            self.assertEqual(hashlib.sha256(font_header).hexdigest(), FONT_HEADER_SHA256)
            with tempfile.TemporaryDirectory(prefix="pt-paula-owner4-checked-stop-") as temporary:
                private = Path(temporary)
                source_pins = source_snapshot(private)
                self.assertEqual(len(source_pins), 91)
                self.assertEqual(set(source_pins), set(FIXTURE_PINS))
                header = private / "pt_font.h"
                header.write_bytes(font_header)
                header.chmod(0o400)
                fixture = private / "fixture"
                environment = {"PATH": "/usr/bin:/bin", "LC_ALL": "C",
                    "TMPDIR": str(private),
                    "ASAN_OPTIONS": "halt_on_error=1:abort_on_error=1",
                    "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
                compiled = subprocess.run(
                    ["/usr/bin/cc", *CFLAGS, *SOURCES, "-o", str(fixture)],
                    cwd=private, env=environment,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    check=True, timeout=120)
                self.assertEqual(compiled.stdout, b"")
                self.assertEqual(compiled.stderr, b"")
                actual = subprocess.run(
                    [str(fixture)], cwd=private, env=environment,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    check=True, timeout=30)
                self.assertEqual(actual.stderr, b"")
                self.assertEqual(actual.stdout, ORACLE.encode("ascii"))
                self.assertEqual(ordinary_pin(header)[0]["sha256"], FONT_HEADER_SHA256)
                verify_sources(source_pins)
        finally:
            verify_sources(source_pins)
            self.assertEqual(preservation_snapshot(), before)


if __name__ == "__main__":
    unittest.main()
