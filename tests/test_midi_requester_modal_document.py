"""Separate private single-case real document admission/modal HOST proposal. NOT RUN."""
from pathlib import Path
import hashlib
import types
import os
import stat
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT
HELPER = CANONICAL / 'tests/test_native_mixed_causal_rolling_entry.py'
HELPER_SHA = 'f73a23edf8c4591dceb44ade3b08d2e1c4c75cd54301b58267d27753f16d58c1'
CFLAGS = ('-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG', '-fsanitize=address,undefined', '-Isrc/core', '-Itests', '-I.')
HOST_FLAGS = ('-DPT_DOCUMENT_TYPED_REPLACEMENT=1', '-DPT_DOCUMENT_TYPED_ACTION=1', '-DPT_DOCUMENT_MIDI_SETUP_HOST_ONLY=1', '-DPT_MIDI_SETUP_OWNER_HOST_ONLY=1', '-DPT_MIDI_REQUESTER_OWNER_HOST_ONLY=1')
SOURCES = ('tests/midi_requester_modal_document_test.c', 'src/native/midi_requester_model.c', 'src/editor/editor_mixed.c', 'src/editor/editor_mixed_checked.c', 'src/editor/editor_mixed_establish.c', 'src/editor/sampler_establish.c', 'src/editor/mixed_owner_established.c', 'src/editor/sampler_prepare_project.c', 'src/editor/sampler_prepare_memory.c', 'src/editor/mixed_owner.c', 'src/editor/mixed_transport.c', 'src/editor/paula_voices.c', 'src/editor/sampler_paula.c', 'src/editor/paula_dispatch.c', 'src/editor/mixed_preflight.c', 'src/editor/paula_preflight.c', 'src/core/paula_render_voice.c', 'src/core/render.c', 'src/core/pitch.c', 'src/core/timeline.c', 'src/core/frame_clock.c', 'src/core/flow.c', 'src/core/voice.c', 'src/core/project.c', 'src/core/channels.c', 'src/core/pcm.c', 'src/core/elapsed_clock.c', 'src/editor/wavetable_song.c', 'src/editor/wavetable_voices.c', 'src/editor/wavetable_dispatch.c', 'src/core/amigus_voice_plan.c', 'src/core/amigus_render_voice.c', 'src/editor/sampler_wavetable.c', 'src/core/amigus_reservation.c', 'src/core/amigus_wavetable_cache.c', 'src/core/amigus_sample_ram.c', 'src/core/sample_cache.c', 'src/core/playback_pcm.c', 'src/editor/sampler.c', 'src/editor/slots.c', 'src/core/pcm_filtered.c', 'src/core/slices.c', 'src/core/pattern.c', 'src/core/document.c', 'src/core/pp20.c', 'src/core/mod_project.c', 'src/core/mod_inspect.c', 'src/core/wav.c', 'src/core/svx.c', 'src/core/raw.c', 'src/editor/sampler_lifetime.c', 'src/core/sample_usage.c', 'src/editor/editor.c', 'src/editor/song.c', 'src/editor/view.c', 'src/editor/sampler_invert_song.c', 'src/core/render_invert.c', 'src/core/invert_bank.c', 'src/core/invert_sequence.c', 'src/core/invert_pcm.c', 'src/core/invert_loop.c', 'src/core/amigus_session.c', 'src/core/amigus_fifo.c', 'src/core/amigus_pcm_pack.c', 'src/core/studio_consumer.c', 'src/core/studio_pump.c', 'src/core/studio_queue.c', 'src/editor/editor_studio.c', 'src/editor/sampler_song.c', 'src/editor/sampler_studio.c', 'src/core/studio_song.c', 'src/core/studio_plan.c', 'src/core/studio_mix.c', 'src/editor/editor_wavetable.c', 'src/platform/sample_import.c', 'src/platform/raw_import.c', 'src/platform/mod_import.c', 'src/platform/pp20_import.c', 'src/editor/workflow.c', 'src/editor/sample_range.c', 'src/editor/wave_summary.c', 'src/core/event_resource.c', 'src/editor/editor_mixed_bridges.c', 'src/editor/editor_document_replacement.c', 'src/editor/editor_document_action.c', 'src/editor/midi_setup.c', 'src/editor/midi_setup_owner.c')
COMMON_FROZEN = {'tests/midi_requester_modal_document_test.c': '65eab5ccc9bf716731cf3c281b2361aa48ecb91d2870d2804e11f7e68a314de7', 'tests/midi_setup_document_test.c': '7a231bcfa6f57487a845fc9d10c995710f64fb5afb03e7732e8fbfd7f9cbb1b6', 'src/editor/midi_setup_owner.h': '5482af8449b8bec62f22d83bfaf432056af158cf4f62b573732373171b4ca17d', 'src/editor/midi_setup.h': '5ee19232be94a6f64843cd74c181249fadad5482dede743383d69b901111abf8', 'src/platform/midi_endpoints.h': '89af1d6d0cd3824e825f3d6b6823bd24d5896db69e8408f004f43dabb5ea5eda', 'src/core/project.h': 'e1a749800258b1f1db1dedcd7d5d36970abbf7494279cb3669212757f982e21a', 'src/core/pcm.h': '9b6996caf7c81bd53034f53e008e7681885ede2711dddf7c2027c3bab06cb10e', 'src/core/channels.h': '9eadeee6befb0f291e62c654a8ccd5ed6c6e7030f8c8c93587566fb06eb99517', 'src/core/midi.h': '0d1cf918cef5a9f53ef2f097e84648be7b753f150e662953db71108025065056', 'src/editor/workflow.h': '1b8519a6152d636e2ddf2bb63221e03f9ffb03aa462363c0ae7edd5f95d0889e', 'src/editor/wave_summary.h': '4a3a18cbd568f953c0078be936c43ae468ed264251eb9600bb4297e6df6c8684', 'src/editor/sample_range.h': '4ceff1c3611c278ef56b9ead9a3674471d0d18ae1d67c4a032593cd8ee211dd8', 'src/core/event_resource.h': 'f0f446bff489a6107ee2d44b904a395c6ab2e4738d8d4f4cff2ac8db070b4d9b', 'src/core/pitch.h': '7ec69432c14acb87bc3ba357fe1031023942ebe4ce35e2193d46c16f7068f8b4', 'src/core/flow.h': 'db5a78efb5344d95f145d70a6c26c664a832410b8ea741c56f0de831d4163046', 'src/editor/sampler_workflow.h': 'a1eff0addfd1fded1185029f68bd9d6d885d5a421b702b40f76cb28af63fac1a', 'src/core/sample_usage.h': '0ed540356b9ff1bd2ae651de2ee183b6c0c45af84f352eb02fab07d9155565a0', 'src/editor/sampler.h': 'c641ee916de9b292c529bf558ce34ea96d3449d4255510a0122cf67e5822fae9', 'src/core/raw.h': 'f9f47added4df41bb7b15aa3b1b7a4d8131a2c4aa19266436318483a41bc0f3b', 'src/core/svx.h': '824541cd25b72b5820e091cfc9f4e6a5f41351fd0630dddc09aefc0405036814', 'src/core/slices.h': '31cd82785c2b86c41d9cde1f7a29056c9d70ec41fb7a9bfb1a5679ba762c24f7', 'src/core/document.h': '9b6613ca8109c59599635ef1fef3b36a90b30703b3e7e3aa4f2674722e69316a', 'src/core/pp20.h': 'd62562e79452f53a0dc9cdc753e7f337d0e305505408199e805f8a43f862dbf6', 'src/core/mod_inspect.h': '9b80158f6e9db0ccde95ba16f58a6ed4292ebae056e500796b074842abf8a29f', 'src/core/pattern.h': 'f8c9abde09c4fe495ae77f25c070da647eab038845729edc83adec08c5678d53', 'src/core/recent.h': '6324b8256d1348ecd3e85d2142da28fcae2a7f8f1f5ce10fb7a1b812dcb6136b', 'src/core/render.h': 'aa531e26bcf3d3fbe6ddfb61a3ea86ae2c1aeb90016e41c4649076a33793bc2f', 'src/core/voice.h': '386b10545fb8ae615b5a45718d90be60b0999b2d63fffd835b6be540d7dca479', 'src/core/timeline.h': 'ef1640b5c7dd9e48e1e13a9963c21c53b913d062b172a988e91abb7532defa64', 'src/core/frame_clock.h': '77bb574a681a492967a6dc938e5eab7e5f28c70f73fd18189de6ed379ccbc54c', 'src/editor/song.h': 'fdcc808b738049b0e47ab8b87bcf556b064511c5da17022f84a33e9d5948e1b2', 'src/core/playback.h': 'e1dc3eca74ac19e73c79b8bc5c793dde3ec367fafb5b8632bf176fef98e3b6a2', 'tests/editor_document_action_focus_test.c': 'c6e010454e6bcaa2660fafff10182bcd5514c9eb7fe06b2afbce550a60596ff3', 'src/core/document_retirement_internal.h': 'eb033fe27358b69706b2772ff72abc6d9910e654a30b73e3725b697f232c65f1', 'src/editor/sampler_lifetime_internal.h': '489b613ba9d848708948bda0a956f79bff249434740a466bc77786f659c7479d', 'src/editor/editor_document_action.h': '866049ef39de097ad429d68185ca7b187aaebb2c55868ba5914f8e001388feb1', 'src/editor/mixed_owner_state_internal.h': '613cab0637234b1309c5adbb96f6d9eaa551a1776c3fae9f2d0f951b6931527a', 'src/core/elapsed_clock.h': 'd5db62de2b938228edb95b1f3efb520d58c5e37a7f10f0b6cca1fc5d8e9ec37e', 'src/core/render_lookahead.h': '6ecdad4402eb721aad8d3b55d6d8fc87a9f8135962c8638f0f1919527c4fbf03', 'src/core/render_commands.h': 'ea176e8382ea808929d11571396d6214bb40d2fce8defc9b19569c1bb5c30d14', 'src/editor/sampler_internal.h': '557dc012f010b66ee8dc2ff87b2fe1d906b3d7d9fbb6685134ae87302dd00f1f', 'src/editor/wavetable_internal.h': '1f865c465e581dc6e762b356454cda3937c91936f741f610985f86c96d5a5c4b', 'src/editor/wavetable_dispatch.h': '2318260d9e94c14826b381b5b339c472e0127564129d578db71414ce89910540', 'src/editor/wavetable_voices.h': '90469885b32f35487c868e4cbcafb01c5abe802eee115bb565fa9918991d06af', 'src/core/amigus_voice_plan.h': '0910081dcb6cd26709c8fc1b5028aa2b0e693a8b48789173dac491b3a511c850', 'src/core/playback_pcm.h': 'b5056427354dc20c166cd6da424b211b206835840555bbd8423b2858bc7d0e65', 'src/core/sample_cache.h': 'df19898830aa7696c38898feae15e3b437679af23d5ea3b69e8aa923e927e5c6', 'src/editor/sampler_wavetable.h': '5b64c95994f3837834b39e3918847f811c1f037cc5a5ba4467031f753783561d', 'src/core/amigus_wavetable_cache.h': '8c449f009eac14807a9521d16a756345dd531e47b50312a3cabf96ee7ea2ee9c', 'src/core/amigus_sample_ram.h': '005111323eb1884112230ea415fbfb75e01d560618326148e08790d4f23fadf6', 'src/core/amigus_reservation.h': 'ee57be55b9865fd19d14b27b22af6e2aa6b0466b7f21da5b313016efe9a0b646', 'src/editor/paula_internal.h': '7338f9b32d3b4ce7ab5a4dd6edcc667cff7d552b1511190d5617bdc23bd135e1', 'src/editor/paula_dispatch.h': '6775ee73d41e615c3a125d3ed1e0a9258496a86dfd7552a2373f044acdc844df', 'src/editor/sampler_paula_internal.h': '18f67906cfc939dfd124c5513864bd4d84185fe913464d2024577af072cfad8d', 'src/editor/sampler_paula.h': '3f25fa64f67c4f6115cbbefd109e6d17e8f648fe9ba955917a6e05f6158a4890', 'src/editor/paula_preflight.h': '3fd4d9096a3592f53503d1c1e2599dea33873b1fd508b41826d8715f6604f67d', 'src/core/paula_render_voice.h': '11126de4c05368ab1acdcfe1f92b56df03b8233f7e2ec0c304c9d5922e913c77', 'src/editor/paula_voices.h': 'e6c685669a391b970617ec2f8b47f1f2781932a398ddabe135c563638e0bba30', 'src/editor/mixed_owner_internal.h': '512c9f5e6c88f869ccc5df0e60122ad5e37ae6c8d558905b7a0cbb3ee72b673f', 'src/editor/mixed_owner.h': '3edb67e1b911e799a91afa59afe7bccbbcbbae9e1e8c009da83e80a02a56f631', 'src/editor/mixed_preflight.h': '805c75ef06d417787cf930a3344dd207f8e34b758e651469aa685599f67afbd2', 'src/editor/editor_document_replacement.h': '004e973651264c60e83e83d9f201f90cd7d2b5632aeba1a79533d7e01902e92e', 'src/editor/editor_mixed_bridges.h': '8065a9b60264dbefacb84b9456bca2caff569eb54005aa6413574f9c02acd7bd', 'src/editor/editor_mixed.h': '36e0c16de159751432870114804ce02e1f53d3eb3cf0860cea49bce35e8b58ce', 'src/editor/mixed_transport.h': '802b95f83f4185bafd5dbfce0b481375d32c4874885195c8292a818493d935f8', 'src/editor/editor_mixed_establish.h': '14e144f9053753be5d84e43962abe6f28237a72e15d966d871ba0fa552dfa7fb', 'src/editor/sampler_establish.h': '55ad8b5202a9100b3ee92bdde3cdd2eb1996de6bcca06bb9b42817208cd0ed88', 'src/editor/sampler_master_guard_internal.h': 'f691504b22dd9250a3442953527470e4dd813a4d8eea900412b2afe1d3ab9a7a', 'src/editor/editor_mixed_checked.h': 'ef0fd56cdc2c73e112a696e9442b5d98563a656865aa110bab022cff78d5e073', 'src/editor/mixed_owner_established.h': 'f00e162e78b3d26028d4e8d9e4bf708bf5f93db80ae63cdd5af0a0b2684a2dac', 'src/editor/sampler_prepare_project.h': '5fd452e8ad652d356a3e3a46c04cebcf6c217277cca59edbf70937b8a62827be', 'src/editor/sampler_prepare_memory.h': '9fdcf4f9ebc9cae45da0a4e61208ff2b06ca0621aa6873b00dc4bbf47ad65146', 'tests/editor_mixed_checked_test.c': '12ba8fdd13e0c5a6cb0baf8a1df6c9dbbefb62136e70e53da6d1071b0973766e', 'src/editor/mixed_owner_established_internal.h': 'ca6e588fb0ba61c2c0f443204c94860fe7508a3fa346fb5986daf03e59a7a00c', 'tests/editor_mixed_test.c': '26836aab616e5121b79c1e31a71f8f8402e56d5297be9af593a08fdae81b9fec', 'src/platform/sample_import.h': '0bcc406620337cb3063976b501ebc362301370b91302c0ddfed170ef7961c2aa', 'tests/mixed_owner_test.c': '23c7cdef1ed75844fb64f415d79f388855266e0366df29f343e23e7b45043332', 'tests/amigus_wavetable_cache_test.c': '0dfb01bd967669b0661ea01cdcec22800dc98f6c0476a83d3995975b83c18eef', 'src/core/playback_internal.h': '266eb7c841cad43ecc34868c219bf723a3b5b836f2f733f6f25f45c68adfbef6', 'tests/amigus_reservation_test.c': 'dad17211f55fa80908f0a01f0f5482a1148946b324267fbd872caeab79205572', 'tests/paula_voices_test.c': '356816d90879b1a10a30e8046475ad754e4bc900b74389f8c4e4192757a1c2d1', 'src/native/midi_requester.h': 'e7da328fde1e40951e747f17a90d4fbe14364f738e6d1cfd2b238f0d0fba08c9', 'src/native/midi_requester_model.c': 'b5ef67da67d6c044ea90e21a007fed80ca7e3c2128292ad05ea0fddeca268316', 'src/editor/editor_mixed.c': 'cd175345384d5aa2f498249d23bc660624820423c734e9d63cb86c7ee0122f25', 'src/editor/editor_mixed_internal.h': 'e376d55fe260d2048210ec491484d6580cce9d61266947b0ef5b269a182fb7b9', 'src/editor/editor_mixed_checked.c': 'a773382bdab50757f8d45d801fc0aa46aab69f67f9106ca50d08608838a7ca2e', 'src/core/render_storage_internal.h': 'ca1eb7fea1a37cba6ebfedd2e91f6e31a0c6d3f02feee383d184e982fad3953e', 'src/editor/editor_mixed_establish.c': '11d4ea87c65d61ba57a01ff3e88eb0d45bfb7463f164dce82a1314a9d68b2e2f', 'src/editor/sampler_establish.c': '840c57fe64a1300d087f6cb465baf038ecd2ad75a98e737b4931185f64aab712', 'src/editor/mixed_owner_established.c': 'f314e756e3062e21361bc539779d50b2a489497ea7f143bda79b44e469aa18d2', 'src/editor/sampler_wavetable_internal.h': 'def1ef4ed705a4b2d3af3030e1072dc7e74bd238bb9f23fbc701241b339fa4df', 'src/editor/sampler_prepare_project.c': '3bf03c11c789640424dedeb9465476be4c68235cd166501ac717c98557b7db2e', 'src/editor/sampler_prepare_memory.c': '436b914d96c53db6280c2b1e77d954f94a634be4534e1cfc0f6035786bf33a89', 'src/editor/mixed_owner.c': '580721245098717a884936c4b0e084d1d1e1453d876c82c1fdc1a92a0cb0b4fb', 'src/editor/project_snapshot.h': 'dbb1206c1e8bfd317b515086f9d064c6eacd2450228893bb394f278ed6c0b2df', 'src/core/amigus_render_voice.h': 'aa3304c76a09c13bddc8476cb1e929f6ee224cc4024f5e9b90ed9ff61b152c77', 'src/editor/mixed_transport.c': 'ec84da17e583b92c217bbd65485177180a895d2db22098fcea69ecf53f64f37d', 'src/editor/paula_voices.c': '514e80086b1510e026cf3bc3936252d0976bb20fe5cc56a0fb0a1a1372008eb2', 'src/editor/sampler_paula.c': 'f0bd08c0c8fb1a9fd46dbf6f813ea1ebf8e769aa55b6b5425c9b875c77fc035d', 'src/editor/paula_dispatch.c': '8e4ae40dcccf418b76cfe90b6ddf5178a1717568eeea9b4afba2da73b3099ec0', 'src/editor/mixed_preflight.c': '394994b3129933cd99008bcefbdd9b4e02db5f5576634886857ac6c944b6b893', 'src/editor/paula_preflight.c': '0e4a6f90cdec366856b0a2325502185746f9a2bab0ff3118d6c5f346dde5a82d', 'src/core/paula_render_voice.c': 'c037e6d3e59d6c898e78ebba05036d960ced3a2758a08cf36adbdcd4780ecc82', 'src/core/render.c': 'acc53af54275d5ec481ca2128dcf72fde305bf91ed28295fd781a81f42966936', 'src/core/voice_internal.h': '02f7f0af75f1a60a7907fbdcbfa754f3d2a06e9073d2bb5e4888f36a66ce5973', 'src/core/pitch.c': '9088859aa6a2812510b3a28334420030add960afa5f2cca3b41946018f0c04d5', 'src/core/pitch_tables.h': '436cb7721c10ef2ef010d7fa6c2b0e46ccec2a4dfc2f7ce9af76a2bb0eb7812a', 'src/core/timeline.c': '2296df1f1d13336bacb40cd867268dbd1c52d1ed0353256f5022da536516effd', 'src/core/frame_clock.c': 'e5d9c6c9dc7ea48daf6d099dc5ade8ec2260d8009f40eebd26fa32bd06490a22', 'src/core/flow.c': 'c1850d6202a01805876ce6afa903ac4ae145757410e7597d70f2601903e79783', 'src/core/voice.c': '9da52c6a82f9ea753655ed4d196946198316cb5327078ffbf354a6dc66e48c68', 'src/core/pcm_internal.h': '1abbf89dfc2b640624d3f2cb16c2c715d818deb4a88f5284383d386a35c0188b', 'src/core/project.c': 'f1bad97dc044e3976936be95c4e42051e8b7a79aab643a1c984a14a1dd24c75a', 'src/core/channels.c': 'b6316f4a19e737d3f5a19bd04f22f43b0da72188d2a0623404dc4a27bc9c622d', 'src/core/pcm.c': 'ddb208ff5dd445c73d1b573eac1aed3e74bc654fefc2662627d4c0d760f99266', 'src/core/elapsed_clock.c': '7db37ec7184235f0776ba773996271641b0d0a8a0efcde3abb1978ab6b414ea8', 'src/editor/wavetable_song.c': 'baeca221d22d3a34732860fbf0cb79150d7947a40bc3d678c60bf8941c6ffc9c', 'src/editor/wavetable_song.h': '8eddcb4b14a0b53337e5d62de5aef53ff737bdaf10b904432e5f0a8b8f547d01', 'src/editor/wavetable_voices.c': '46768220e60a19fdddfa6a45d6d7d91928b0d40db00c1bceb45bc7aac22d1cd4', 'src/editor/wavetable_dispatch.c': 'ebeadff7f05217af3f66bbe514053022638195e806a70b25c7e73a2ed636ea64', 'src/core/amigus_voice_plan.c': '289c7c3a6b8b1932d219bc61602ba6bd925cbfd3b24e95cbc8aa6f1642134964', 'src/core/amigus_render_voice.c': 'b777facd29ca03e8e247697cec4b71f69af0d71cd6e9508eee7270be3c98ca3a', 'src/editor/sampler_wavetable.c': '98784fff3238d96b91685f43a2b41f7bc93fd3be49f91eda5599a9089f3e634b', 'src/core/amigus_reservation.c': '9d448f569e2f86331085343626e2b653e45c1fb96aaaf6c709f0eefe168e1e27', 'src/core/amigus_wavetable_cache.c': 'c634faa6c6c6af0086ec62a92e49ee2d6b2f90adbb0ebd0acabb901c497cae8d', 'src/core/amigus_sample_ram.c': '5c25d93bf9166b568fccf599b392a4ca11c24923efc915f0dd4e4dfad7e1fa9a', 'src/core/sample_cache.c': 'ebe0d55d650ae0bc0aed9f5a5628dca942a6914082c25366dfe52dcdddbbf39a', 'src/core/playback_pcm.c': '8c6847671c90fa8af25ba696cc16899e7a0104465c2cf9e4fc2dc269cda28c88', 'src/editor/sampler.c': 'd3bba34d4725a6f9cc311fdf3ba2d2233ed84f9ba43d57dd1bc92226446bdc57', 'src/core/mod_project.h': 'c28407161729fc0fea2398f79341a866f03913b092769674bd871bb23adb59ee', 'src/core/wav.h': '8dd0f4454304babb53eef5ecb97cde7bfe6569d292cc0e9a0f204c4a587f2028', 'src/editor/slots.c': 'ac9e10cdf165688b98d4a5562966fb7eee8e8a52376de065e3c61251f57045f8', 'src/core/pcm_filtered.c': '198e436dd49fbc3984cc612554961bd70debad6b8ba4b4b9b4daa8020715a5c9', 'src/core/sinc_kernel.h': '058629396a0b22daa67f75c909adf9181fc4e52bfc668890fe2abc3231c3a185', 'src/core/slices.c': '2951045844f6fdebdd80afe12fe7a5afc1f37bf837c2f49d840f14867835342f', 'src/core/pattern.c': '0d22aff4b63fae85fa44f063ba6c2eeba37467843e4fcceb493037036366431e', 'src/core/document.c': '64973ff3b3f2b2806ea523c752244689d3275aa9ec66b964bf4db74fea72e2b6', 'src/core/pp20.c': '3f03dac4f224f54a72437370d94e1cd09ce9222a3748a0e805181ae26c5f1465', 'src/core/mod_project.c': '793c9ece248b586dd75e7b3ffefefe446bfbc6d4cc476fc2a61a4a1ce8877b09', 'src/core/mod_inspect.c': '278f601c17ad72a095e6eb17c4f5bbe321bf342edebacf7598ca830f2743b511', 'src/core/wav.c': '499264a12362a271c20dcc13f4e5e38a470aa5a4ddce90a44c5193f4d8be916f', 'src/core/svx.c': '4712a8d846dd8e5a36fbbdb9b2db7c2fc71ed215be6b114da7d0285cebf69813', 'src/core/raw.c': '460fe21a893b38f14e4af797022f6409504b125f562509b59efef713dc704377', 'src/editor/sampler_lifetime.c': '975e50324af6d280b7d4c82a75a41d56d1bc940412a14ff2ba89a676887700bb', 'src/core/sample_usage.c': '14520084e266041ea257bc7e1b5145f9322c5549e4e4377f5b962232f089b160', 'src/editor/song.c': '0e6c75429891da87865e3e2242057401db81129b87a4963dc3730f3f1786559c', 'src/editor/sampler_invert_song.c': '5aef31d4ea1bef6f122712702234469527b403ad724afa1822588c39101ca5a9', 'src/editor/sampler_invert_song.h': '2d6fad60faf1803755adf91b1952df41ab9b7367a73514038337767f7a3aac14', 'src/core/render_invert.h': '9d66ec381257454dc816d39ba0ee5b07db4d1f09f326cdcdbd8abbf66285e6d5', 'src/core/render_invert.c': 'aa36e316b315c85d357ab8e8ce9cd54d810899481c4ffafe313272d8bf34bc81', 'src/core/invert_sequence.h': '2b8c74b91ac2ad8d15b54f3ee73a7945668794c9218850aeaa6f11f3ed9302ef', 'src/core/invert_pcm.h': 'd521c050aafee6e808d3312d81ecaa958ac957ef61f5f98f41678d1f4070174d', 'src/core/invert_loop.h': 'bce84104f8850fa8272ab20f614be7fa226e51be5ea243ec7d7c32afdae9d90a', 'src/core/invert_bank.h': '5d55e42986389b2b8f6d9d3b80fa8d51603a6e9e00e014d185aaae21ef53d377', 'src/core/invert_bank.c': 'b374b4e88ead336b6c7566038c5a2f090552fc3e562751f9ef7ec2a931005d5f', 'src/core/invert_sequence.c': 'f290f38c685ba81b0b2503e52ae43071021d25f00c6c8ecc8eda12f95086fa51', 'src/core/invert_pcm.c': 'f9b6b4259468f27cab2064831d1d62efd28c751c51de59073baf4b39e59c7404', 'src/core/invert_loop.c': '3df53bcdf5851e16aae2bdd34d2ee97c7c0b4ae4fd1333c53b4994f0a3c0e852', 'src/core/amigus_session.c': '672fc5b6e2b720fb74db90040be6951eb92fc894e78351b8ab6c5a9917113460', 'src/core/amigus_session.h': '43b5a5ab74dcff269c692013fca4feb7145d1c6f20c0971a80321749c6ec047c', 'src/core/studio_consumer.h': '2b081296f92f2f49961b16d09c18bf169795ba598797d4cec088b3f4f890750e', 'src/core/studio_queue.h': '891f680cb1cce81943e864cf73592a6b65643203762218c7676073935bc4bb04', 'src/core/amigus_fifo.h': 'b218313c6b243a678bf3fd12bc290ea12ff6d28aa9179677c9c5aec628020db2', 'src/core/amigus_pcm_pack.h': 'd0648c43fef1e70ffe92c7540be04e9c70e219bb2ee4cda9bf2ef1357e9b89bb', 'src/core/amigus_fifo.c': '5c14614a81f05cdac1820be120700b9d9ed29750d1e26cc621fb1f6fadad2cef', 'src/core/amigus_pcm_pack.c': '3464933782918baab33ee8e1ca76a7e959897284b98e24015625707cefdfcac9', 'src/core/studio_consumer.c': 'fe871a573e0746205a80adf051252ec8429ffc013f00c20d4980f5d9ee324573', 'src/core/studio_pump.c': 'c62e46bbff6556ef89197f75383c885c81a96bd9dc8b393b1a1e9539f38d2d4d', 'src/core/studio_pump.h': '679d0b48d09696c11cfdfd5fe4033fb7c2f4ea8784073fadb13604d0331aae16', 'src/core/studio_queue.c': '31a58a5e514bce52606e875e642865db42676cec0dc84d3aae3c4b5e46b5b7e5', 'src/editor/editor_studio.c': '4a7a98f4fc6f8c5e77c4856a0ebb30360ca0c32bc3ea00930a9b53de27df2c7d', 'src/editor/editor_studio.h': 'eca7c1d52084755edd0b239060114ef8455142ed5d5f840b5b1e8709691265c4', 'src/editor/sampler_song.h': '0e74dbc5cfc6a5371420ad789f551f2e8d1e9f1ef48424efa768059f57281d26', 'src/core/studio_song.h': '57e7c0d2b240263ffe79e86fcc4daaf31bcf9149fa19d0fe7ddab8bf944cd57d', 'src/core/studio_plan.h': '4fbd66a0c7d7c08d6324242f00488e0a3a017f925ce8010ae9f8b11d19f33bdc', 'src/core/studio_mix.h': 'eda353006ca5bad15eae65c82e59d79ea6e4593f9c49beace52d489a6aa7278e', 'src/editor/sampler_song.c': 'ebba4a380a76b9a367fe28d84c5854de765e89b527c4220c48906946b681d0aa', 'src/core/studio_internal.h': '728e1116526c7ae0132049c0d649b1d06585fb0119491077c904f85fa6a0595e', 'src/editor/sampler_studio.c': '33849a9ebd51025135c25f0e96f5d31a3f5a9d507176e343a02dbe973439fbab', 'src/editor/sampler_studio.h': '894dc0198f7c1f01f1be82eae671efb6e2f6abaeeb6bd1bb25415b6114108874', 'src/core/studio_song.c': '8271adb666712681e8cb31730dc2e548437039f3001baafd087b2e0684350b43', 'src/core/studio_plan.c': 'f594a572f7a33af7f5167f2223c61e894b7a5ea7d8289e1910667d41279da5fe', 'src/core/studio_mix.c': '21fd2e99644680be922d9f0a4b713544c8234ecf42212a74e1ffba5298f23e28', 'src/editor/editor_wavetable.c': '17bc84e6cca8767179aa8a106822539cf9a589d94fd8839fc9734966df2b4ec9', 'src/editor/editor_wavetable.h': '875be33ae99dabd218ec0b577ffeef0205d3ae933ebe6a9ed48b084034c787b8', 'src/platform/sample_import.c': '0fee44980b58c88b55e48480b3a6e53807e36cbd47fce670bba53d00bd28e300', 'src/platform/pp20_import.h': 'da66dc8b7f278cd9a9acd40347a0c8e42ed1b9f6672e02f7712a94e12e6b2e96', 'src/platform/mod_import.h': 'a4e0607155f5c3b3420c6e9767668a9583e9e16e17960903286dc6ec3185c6db', 'src/platform/raw_import.h': 'f33944e9c16ed119eaceecb4074575ad5d26a5917a009d52ffd671b7dd8814e5', 'src/platform/raw_import.c': 'a4286e55accb510e4deedbb4f1a749c7a74010b3c3adb8318c3fbbf9be93c848', 'src/platform/mod_import.c': '855e2d3633e83397cf3232aba83c89f044f64f0f09aa5345a9208a638ff870ba', 'src/platform/pp20_import.c': '1dcf59769e988a0ab802dbc808e45b8097f47cd46f53510dd4fd391c5c535822', 'src/editor/workflow.c': '768dba18085aeb8e5c815a558ecf233f57652bd5762bbbe866d0ffbce519b6ca', 'src/editor/sample_range.c': '694e14d1a665f2a9b5c844a40e79a27ad1c56eedd5a2c6e379aace8f235980c4', 'src/editor/wave_summary.c': '3e4501324e5888cf6c7b3497c7c6dcb60ef8dcf75e4a53d4d57b1222ae06ff34', 'src/core/event_resource.c': '77b4dd85cd2240c8ac0aa9037a5acfcbccc3fa7c70ae5e548c406fe063846970', 'src/editor/editor_mixed_bridges.c': '95007dc1c350def24801797bc116d17d6a2f7811e44ec39166be6ba66e99810a', 'src/editor/editor_document_replacement.c': 'de6ab36ab44a29900ea9dacd061183adefc4ecb33de30bcb9faff48b4212744b', 'src/editor/editor_document_action.c': '259de6c492990053ba27bfe8d08934b4636a59b21925b592345dd66b89c3c91c', 'src/editor/midi_setup.c': 'ff254db741b4d0c98b289f2e1c2644ccd8084db55a0ec01f0dafdefdf3c5fa38', 'src/editor/midi_setup_owner.c': '959d64e0166c3b0ea78f4f821b9754be0eba1afebf65e731022ef4641705af47'}
ORACLE = 'MIDI REQUESTER DOCUMENT HOST PASS:1 case;real modal blocks action;return prepares/cancels;redo/save/positive disposal;NO NATIVE AUTHORITY\n'

QUARTET_INPUTS = ('src/editor/editor.c', 'src/editor/editor.h', 'src/editor/view.c', 'src/editor/view.h')
CLEAN_GIT_EDITOR_VIEW_QUARTET = {'src/editor/editor.c': {'bytes': 69993, 'sha256': 'c569b9bb445b4af2df5bbe44f6d2fd07166c9d9c668272098aaa8ead32f47ec0'}, 'src/editor/editor.h': {'bytes': 6646, 'sha256': '5b93982695779bafe45a1093cd3b3f4c905524dda5a6556a00f410e528f7aa65'}, 'src/editor/view.c': {'bytes': 42964, 'sha256': '5ae31dfe5d90f5dc6214a9ccad84751779c5f02b8efce196bdbd48cbb3002a06'}, 'src/editor/view.h': {'bytes': 1731, 'sha256': '1ce9950cfa9efa124e5a887dc6db36bebc43c91b410dfbc9ce8dd60c1b324d6c'}}
WORKING_EDITOR_VIEW_QUARTET = {'src/editor/editor.c': {'bytes': 69953, 'sha256': '6301ed250aa5f0518b360c448a2fb742ab48fdc8e828c1a2d3c69df3ab479ea6'}, 'src/editor/editor.h': {'bytes': 6699, 'sha256': '63fb2ab5b528b327a347811dc446bb95fbabf92a873199b832583380ddcec6c3'}, 'src/editor/view.c': {'bytes': 44217, 'sha256': 'c8925fbb936a48304219be5f055327e083c309d0cc3b512d02e88c8703e31432'}, 'src/editor/view.h': {'bytes': 2032, 'sha256': '44cdc9126cd9331d8cb0e428286f6fe1165187fb68c79a809cc6df54852a59b2'}}
KNOWN_QUARTETS = (('COMPLETE_CLEAN_GIT_QUARTET', CLEAN_GIT_EDITOR_VIEW_QUARTET),
                  ('COMPLETE_CURRENT_WORKING_QUARTET', WORKING_EDITOR_VIEW_QUARTET))


def select_editor_view_quartet(observed):
    """Pure exact two-quartet admission; no arbitrary body/hash override."""
    assert set(observed) == set(QUARTET_INPUTS), 'complete editor/view C/H quartet required'
    for kind, expected in KNOWN_QUARTETS:
        if all(observed[name]['bytes'] == expected[name]['bytes'] and
               observed[name]['sha256'] == expected[name]['sha256'] for name in QUARTET_INPUTS):
            return kind, expected
    raise AssertionError('unknown or mixed editor/view quartet; compiler admission refused')


class FrontendQuartetAdmission(unittest.TestCase):
    def test_two_complete_known_quartets(self):
        for kind, quartet in KNOWN_QUARTETS:
            with self.subTest(kind=kind):
                observed = {name: dict(pin) for name,pin in quartet.items()}
                self.assertEqual(select_editor_view_quartet(observed), (kind, quartet))

    def test_all_fourteen_mixed_quartets_refuse(self):
        for mask in range(1, 15):
            with self.subTest(mixed=mask):
                observed = {name: dict((WORKING_EDITOR_VIEW_QUARTET if mask & (1 << n)
                    else CLEAN_GIT_EDITOR_VIEW_QUARTET)[name]) for n,name in enumerate(QUARTET_INPUTS)}
                before = {name: dict(pin) for name,pin in observed.items()}
                with self.assertRaises(AssertionError):
                    select_editor_view_quartet(observed)
                self.assertEqual(observed, before)

    def test_each_unknown_hash_and_size_refuse(self):
        for name in QUARTET_INPUTS:
            for field in ('sha256', 'bytes'):
                with self.subTest(body=name, field=field):
                    observed = {body: dict(pin) for body,pin in WORKING_EDITOR_VIEW_QUARTET.items()}
                    observed[name][field] = '0' * 64 if field == 'sha256' else observed[name][field] + 1
                    before = {body: dict(pin) for body,pin in observed.items()}
                    with self.assertRaises(AssertionError):
                        select_editor_view_quartet(observed)
                    self.assertEqual(observed, before)

    def test_missing_or_extra_body_refuse(self):
        missing = {body: dict(pin) for body,pin in CLEAN_GIT_EDITOR_VIEW_QUARTET.items()}
        del missing[QUARTET_INPUTS[-1]]
        extra = {body: dict(pin) for body,pin in CLEAN_GIT_EDITOR_VIEW_QUARTET.items()}
        extra['unknown'] = dict(next(iter(extra.values())))
        for observed in (missing, extra):
            with self.assertRaises(AssertionError):
                select_editor_view_quartet(observed)


def load_pinned_helper():
    before = HELPER.lstat()
    assert stat.S_ISREG(before.st_mode) and not before.st_flags & 0x40000000
    names = ('st_dev', 'st_ino', 'st_mode', 'st_nlink', 'st_size', 'st_uid',
             'st_gid', 'st_mtime_ns', 'st_ctime_ns', 'st_flags')
    facts = lambda value: tuple(getattr(value, name, 0) for name in names)
    descriptor = os.open(HELPER, os.O_RDONLY | os.O_NOFOLLOW)
    try:
        assert facts(os.fstat(descriptor)) == facts(before)
        body = bytearray()
        while True:
            chunk = os.read(descriptor, 65536)
            if not chunk:
                break
            body.extend(chunk)
        assert facts(os.fstat(descriptor)) == facts(before)
    finally:
        os.close(descriptor)
    assert facts(HELPER.lstat()) == facts(before)
    assert len(body) == before.st_size and hashlib.sha256(body).hexdigest() == HELPER_SHA
    base = types.ModuleType('pt_midi_setup_document_frozen_helper')
    base.__file__ = str(HELPER)
    exec(compile(bytes(body), str(HELPER), 'exec'), base.__dict__)
    assert base.ROOT == CANONICAL and base.CFLAGS == CFLAGS
    return base


class MidiRequesterDocumentModalHost(unittest.TestCase):
    def test_one_real_document_admission_and_cancel(self):
        base = load_pinned_helper()
        before = base.preservation_snapshot()
        base.evidence('requester-modal-document-before-preservation.json', before)
        helper_before, _ = base.ordinary_pin(HELPER)
        wrapper_before, _ = base.ordinary_pin(Path(__file__))
        old_units = base.SOURCES
        original_inputs = {}
        pins = {}
        try:
            self.assertEqual((len(SOURCES), len(set(SOURCES)), len(COMMON_FROZEN)), (87, 87, 201))
            observed = {name: base.ordinary_pin(ROOT / name)[0] for name in QUARTET_INPUTS}
            quartet_kind, quartet = select_editor_view_quartet(observed)
            frozen = dict(COMMON_FROZEN)
            frozen.update({name: pin['sha256'] for name,pin in quartet.items()})
            self.assertEqual(len(frozen), 205)
            for name, sha in frozen.items():
                fact, _ = base.ordinary_pin(ROOT / name)
                self.assertEqual(fact['sha256'], sha)
                original_inputs[name] = fact
            base.ROOT = ROOT
            base.SOURCES = SOURCES
            with tempfile.TemporaryDirectory(prefix='pt-requester-modal-document-') as temporary:
                private = Path(temporary)
                pins = base.source_snapshot(private)
                self.assertEqual(set(pins), set(frozen))
                for name, fact in pins.items():
                    self.assertEqual(fact['sha256'], frozen[name])
                environment = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(private),
                    'ASAN_OPTIONS': 'halt_on_error=1:abort_on_error=1',
                    'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
                executable = private / 'fixture'
                timings = {}
                commands = (
                    ('compile', ['/usr/bin/cc', *CFLAGS, *HOST_FLAGS, *SOURCES, '-o', str(executable)], 120, b''),
                    ('fixture', [str(executable)], 30, ORACLE.encode('ascii')))
                for name, command, timeout, oracle in commands:
                    prefix = 'requester-modal-document-'+name
                    base.evidence(prefix+'-command.json', {'argv': command, 'cwd': str(private),
                        'env': environment, 'timeoutSeconds': timeout})
                    start = time.monotonic()
                    try:
                        result = subprocess.run(command, cwd=private, env=environment,
                            capture_output=True, timeout=timeout)
                    except subprocess.TimeoutExpired as error:
                        base.evidence(prefix+'.stdout', error.stdout or b'')
                        base.evidence(prefix+'.stderr', error.stderr or b'')
                        raise
                    timings[name] = time.monotonic()-start
                    base.evidence(prefix+'.stdout', result.stdout)
                    base.evidence(prefix+'.stderr', result.stderr)
                    self.assertEqual((result.returncode, result.stdout, result.stderr), (0, oracle, b''))
                base.evidence('requester-modal-document-inputs.json', {'sources': pins,
                    'orderedTranslationUnits': SOURCES, 'literalProjectInputs': 205, 'flags': HOST_FLAGS,
                    'editorViewQuartet': quartet, 'editorViewQuartetKind': quartet_kind,
                    'helperPin': helper_before, 'wrapperPin': wrapper_before, 'timingsSeconds': timings,
                    'cases': 1, 'pureQuartetCases': 26, 'requiredRootOuterTimeoutSeconds': 180,
                    'scope': 'Separate real modal/action admission and prepare-cancel case only. Actual history redo occurs only after genuine original Stop. No publication/repeating native session/output/quiet/target authority.'})
                base.verify_sources(pins)
        finally:
            base.ROOT = ROOT
            base.SOURCES = old_units
            base.verify_sources(pins)
            for name, fact in original_inputs.items():
                self.assertEqual(base.stable_fact(base.ordinary_pin(ROOT / name)[0]), base.stable_fact(fact))
            self.assertEqual(base.stable_fact(helper_before), base.stable_fact(base.ordinary_pin(HELPER)[0]))
            self.assertEqual(base.stable_fact(wrapper_before), base.stable_fact(base.ordinary_pin(Path(__file__))[0]))
            base.ROOT = CANONICAL
            after = base.preservation_snapshot()
            base.evidence('requester-modal-document-after-preservation.json', after)
            self.assertEqual(base.stable_fact(before), base.stable_fact(after))


if __name__ == '__main__':
    unittest.main()
