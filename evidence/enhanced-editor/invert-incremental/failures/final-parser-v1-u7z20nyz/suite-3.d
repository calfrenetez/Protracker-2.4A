editor_studio_test.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  tests/editor_studio_test.c tests/../src/editor/editor_studio.h \
  tests/../src/editor/editor.h src/core/pattern.h src/core/project.h \
  src/core/channels.h src/core/pcm.h src/core/playback.h \
  tests/../src/editor/sampler.h src/core/document.h \
  src/core/mod_inspect.h src/core/pp20.h src/core/slices.h \
  src/core/svx.h src/core/raw.h tests/../src/editor/song.h \
  src/core/render.h src/core/timeline.h src/core/flow.h \
  src/core/frame_clock.h src/core/voice.h src/core/recent.h \
  tests/../src/editor/sampler_song.h \
  tests/../src/editor/../core/studio_song.h \
  tests/../src/editor/../core/studio_plan.h \
  tests/../src/editor/../core/render_commands.h \
  tests/../src/editor/../core/render.h \
  tests/../src/editor/../core/pitch.h tests/../src/editor/../core/flow.h \
  tests/../src/editor/../core/studio_mix.h \
  tests/../src/editor/../core/document.h \
  tests/../src/editor/../core/voice.h \
  tests/../src/editor/sampler_invert_song.h \
  tests/../src/editor/../core/render_invert.h \
  tests/../src/editor/../core/studio_pump.h \
  tests/../src/editor/../core/studio_queue.h \
  tests/../src/editor/../core/pcm.h \
  tests/../src/editor/../core/amigus_session.h \
  tests/../src/editor/../core/amigus_fifo.h \
  tests/../src/editor/../core/amigus_pcm_pack.h \
  tests/../src/editor/../core/studio_consumer.h \
  tests/../src/editor/../core/amigus_reservation.h \
  tests/../src/core/amigus_session.h tests/editor_studio_output_cases.h
amigus_reservation.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/amigus_reservation.c src/core/amigus_reservation.h
sampler_invert_song.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/editor/sampler_invert_song.c src/editor/sampler_invert_song.h \
  src/editor/sampler.h src/core/pattern.h src/core/project.h \
  src/core/channels.h src/core/pcm.h src/core/document.h \
  src/core/mod_inspect.h src/core/pp20.h src/core/slices.h \
  src/core/svx.h src/core/raw.h src/editor/../core/render_invert.h \
  src/editor/../core/render.h src/editor/../core/timeline.h \
  src/editor/../core/flow.h src/editor/../core/project.h \
  src/editor/../core/frame_clock.h src/editor/../core/voice.h \
  src/editor/../core/pcm.h src/editor/project_snapshot.h
render_invert.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/render_invert.c src/core/render_invert.h src/core/render.h \
  src/core/timeline.h src/core/flow.h src/core/project.h \
  src/core/channels.h src/core/pcm.h src/core/frame_clock.h \
  src/core/voice.h src/core/render_commands.h src/core/pitch.h \
  src/core/invert_bank.h src/core/document.h src/core/mod_inspect.h \
  src/core/pp20.h src/core/invert_pcm.h src/core/invert_loop.h \
  src/core/invert_sequence.h
invert_bank.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/invert_bank.c src/core/invert_bank.h src/core/document.h \
  src/core/project.h src/core/channels.h src/core/pcm.h \
  src/core/mod_inspect.h src/core/pp20.h src/core/invert_pcm.h \
  src/core/invert_loop.h src/core/pcm_internal.h
invert_sequence.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/invert_sequence.c src/core/invert_sequence.h src/core/flow.h \
  src/core/project.h src/core/channels.h src/core/pcm.h \
  src/core/invert_pcm.h src/core/invert_loop.h
invert_pcm.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/invert_pcm.c src/core/invert_pcm.h src/core/pcm.h \
  src/core/invert_loop.h src/core/pcm_internal.h
invert_loop.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/invert_loop.c src/core/invert_loop.h
amigus_session.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/amigus_session.c src/core/amigus_session.h \
  src/core/amigus_fifo.h src/core/amigus_pcm_pack.h src/core/pcm.h \
  src/core/studio_consumer.h src/core/studio_queue.h src/core/document.h \
  src/core/project.h src/core/channels.h src/core/mod_inspect.h \
  src/core/pp20.h
amigus_fifo.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/amigus_fifo.c src/core/amigus_fifo.h \
  src/core/amigus_pcm_pack.h src/core/pcm.h
amigus_pcm_pack.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/amigus_pcm_pack.c src/core/amigus_pcm_pack.h src/core/pcm.h
studio_consumer.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/studio_consumer.c src/core/studio_consumer.h \
  src/core/studio_queue.h src/core/document.h src/core/project.h \
  src/core/channels.h src/core/pcm.h src/core/mod_inspect.h \
  src/core/pp20.h
studio_pump.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/studio_pump.c src/core/studio_pump.h src/core/studio_queue.h \
  src/core/document.h src/core/project.h src/core/channels.h \
  src/core/pcm.h src/core/mod_inspect.h src/core/pp20.h \
  src/core/render.h src/core/timeline.h src/core/flow.h \
  src/core/frame_clock.h src/core/voice.h
studio_queue.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/studio_queue.c src/core/studio_queue.h src/core/document.h \
  src/core/project.h src/core/channels.h src/core/pcm.h \
  src/core/mod_inspect.h src/core/pp20.h
editor_studio.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/editor/editor_studio.c src/editor/editor_studio.h \
  src/editor/editor.h src/core/pattern.h src/core/project.h \
  src/core/channels.h src/core/pcm.h src/core/playback.h \
  src/editor/sampler.h src/core/document.h src/core/mod_inspect.h \
  src/core/pp20.h src/core/slices.h src/core/svx.h src/core/raw.h \
  src/editor/song.h src/core/render.h src/core/timeline.h \
  src/core/flow.h src/core/frame_clock.h src/core/voice.h \
  src/core/recent.h src/editor/sampler_song.h \
  src/editor/../core/studio_song.h src/editor/../core/studio_plan.h \
  src/editor/../core/render_commands.h src/editor/../core/render.h \
  src/editor/../core/pitch.h src/editor/../core/flow.h \
  src/editor/../core/studio_mix.h src/editor/../core/document.h \
  src/editor/../core/voice.h src/editor/sampler_invert_song.h \
  src/editor/../core/render_invert.h src/editor/../core/studio_pump.h \
  src/editor/../core/studio_queue.h src/editor/../core/pcm.h \
  src/editor/../core/amigus_session.h src/editor/../core/amigus_fifo.h \
  src/editor/../core/amigus_pcm_pack.h \
  src/editor/../core/studio_consumer.h \
  src/editor/../core/amigus_reservation.h
sampler_song.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/editor/sampler_song.c src/editor/sampler_song.h \
  src/editor/sampler.h src/core/pattern.h src/core/project.h \
  src/core/channels.h src/core/pcm.h src/core/document.h \
  src/core/mod_inspect.h src/core/pp20.h src/core/slices.h \
  src/core/svx.h src/core/raw.h src/editor/../core/studio_song.h \
  src/editor/../core/studio_plan.h src/editor/../core/render_commands.h \
  src/editor/../core/render.h src/editor/../core/timeline.h \
  src/editor/../core/flow.h src/editor/../core/project.h \
  src/editor/../core/frame_clock.h src/editor/../core/voice.h \
  src/editor/../core/pcm.h src/editor/../core/pitch.h \
  src/editor/../core/studio_mix.h src/editor/../core/document.h \
  src/editor/project_snapshot.h src/editor/sampler_internal.h \
  src/editor/../core/studio_internal.h
sampler_studio.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/editor/sampler_studio.c src/editor/sampler_studio.h \
  src/editor/sampler.h src/core/pattern.h src/core/project.h \
  src/core/channels.h src/core/pcm.h src/core/document.h \
  src/core/mod_inspect.h src/core/pp20.h src/core/slices.h \
  src/core/svx.h src/core/raw.h src/editor/../core/studio_mix.h \
  src/editor/../core/document.h src/editor/../core/voice.h \
  src/editor/../core/pcm.h
studio_song.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/studio_song.c src/core/studio_internal.h \
  src/core/studio_song.h src/core/studio_plan.h \
  src/core/render_commands.h src/core/render.h src/core/timeline.h \
  src/core/flow.h src/core/project.h src/core/channels.h src/core/pcm.h \
  src/core/frame_clock.h src/core/voice.h src/core/pitch.h \
  src/core/studio_mix.h src/core/document.h src/core/mod_inspect.h \
  src/core/pp20.h
studio_plan.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/studio_plan.c src/core/studio_internal.h \
  src/core/studio_song.h src/core/studio_plan.h \
  src/core/render_commands.h src/core/render.h src/core/timeline.h \
  src/core/flow.h src/core/project.h src/core/channels.h src/core/pcm.h \
  src/core/frame_clock.h src/core/voice.h src/core/pitch.h \
  src/core/studio_mix.h src/core/document.h src/core/mod_inspect.h \
  src/core/pp20.h
render.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/render.c src/core/render.h src/core/timeline.h \
  src/core/flow.h src/core/project.h src/core/channels.h src/core/pcm.h \
  src/core/frame_clock.h src/core/voice.h src/core/render_commands.h \
  src/core/pitch.h src/core/render_lookahead.h src/core/voice_internal.h \
  src/core/document.h src/core/mod_inspect.h src/core/pp20.h
pitch.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/pitch.c src/core/pitch.h src/core/flow.h src/core/project.h \
  src/core/channels.h src/core/pcm.h src/core/pitch_tables.h
timeline.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/timeline.c src/core/timeline.h src/core/flow.h \
  src/core/project.h src/core/channels.h src/core/pcm.h \
  src/core/frame_clock.h
frame_clock.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/frame_clock.c src/core/frame_clock.h
flow.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/flow.c src/core/flow.h src/core/project.h src/core/channels.h \
  src/core/pcm.h
studio_mix.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/studio_mix.c src/core/studio_internal.h \
  src/core/studio_song.h src/core/studio_plan.h \
  src/core/render_commands.h src/core/render.h src/core/timeline.h \
  src/core/flow.h src/core/project.h src/core/channels.h src/core/pcm.h \
  src/core/frame_clock.h src/core/voice.h src/core/pitch.h \
  src/core/studio_mix.h src/core/document.h src/core/mod_inspect.h \
  src/core/pp20.h src/core/voice_internal.h
voice.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/voice.c src/core/voice_internal.h src/core/voice.h \
  src/core/pcm.h src/core/pcm_internal.h
editor.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/editor/editor.c src/editor/editor.h src/core/pattern.h \
  src/core/project.h src/core/channels.h src/core/pcm.h \
  src/core/playback.h src/editor/sampler.h src/core/document.h \
  src/core/mod_inspect.h src/core/pp20.h src/core/slices.h \
  src/core/svx.h src/core/raw.h src/editor/song.h src/core/render.h \
  src/core/timeline.h src/core/flow.h src/core/frame_clock.h \
  src/core/voice.h src/core/recent.h
song.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/editor/song.c src/editor/song.h src/core/document.h \
  src/core/project.h src/core/channels.h src/core/pcm.h \
  src/core/mod_inspect.h src/core/pp20.h src/core/pattern.h
view.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/editor/view.c src/editor/view.h src/editor/editor.h \
  src/core/pattern.h src/core/project.h src/core/channels.h \
  src/core/pcm.h src/core/playback.h src/editor/sampler.h \
  src/core/document.h src/core/mod_inspect.h src/core/pp20.h \
  src/core/slices.h src/core/svx.h src/core/raw.h src/editor/song.h \
  src/core/render.h src/core/timeline.h src/core/flow.h \
  src/core/frame_clock.h src/core/voice.h src/core/recent.h
sampler.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/editor/sampler.c src/editor/sampler.h src/core/pattern.h \
  src/core/project.h src/core/channels.h src/core/pcm.h \
  src/core/document.h src/core/mod_inspect.h src/core/pp20.h \
  src/core/slices.h src/core/svx.h src/core/raw.h \
  src/editor/sampler_internal.h src/editor/../core/pcm_internal.h \
  src/editor/../core/pcm.h src/core/wav.h
slots.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/editor/slots.c src/editor/sampler.h src/core/pattern.h \
  src/core/project.h src/core/channels.h src/core/pcm.h \
  src/core/document.h src/core/mod_inspect.h src/core/pp20.h \
  src/core/slices.h src/core/svx.h src/core/raw.h src/core/mod_project.h
pcm_filtered.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/pcm_filtered.c src/core/pcm.h src/core/sinc_kernel.h
slices.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/slices.c src/core/slices.h src/core/pcm.h
wav.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/wav.c src/core/wav.h src/core/pcm.h
svx.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/svx.c src/core/svx.h src/core/pcm.h
raw.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/raw.c src/core/raw.h src/core/pcm.h
document.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/document.c src/core/document.h src/core/project.h \
  src/core/channels.h src/core/pcm.h src/core/mod_inspect.h \
  src/core/pp20.h src/core/mod_project.h
pp20.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/pp20.c src/core/pp20.h
pattern.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/pattern.c src/core/pattern.h src/core/project.h \
  src/core/channels.h src/core/pcm.h
project.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/project.c src/core/project.h src/core/channels.h \
  src/core/pcm.h
mod_project.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/mod_project.c src/core/mod_project.h src/core/project.h \
  src/core/channels.h src/core/pcm.h src/core/mod_inspect.h
mod_inspect.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/mod_inspect.c src/core/mod_inspect.h
channels.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/channels.c src/core/channels.h
pcm.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/pcm.c src/core/pcm_internal.h src/core/pcm.h
