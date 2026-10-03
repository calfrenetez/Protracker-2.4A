native_exec_editor_capture_test.o: \
 tests/native_exec_editor_capture_test.c tests/native_exec_memory.h \
 tests/../src/native/master_memory.h tests/editor_capture_test.c \
 tests/amigus_capture_test.c tests/capture_session_test.c \
 tests/../src/core/capture_session.h tests/../src/core/capture.h \
 tests/../src/core/document.h tests/../src/core/project.h \
 tests/../src/core/channels.h tests/../src/core/pcm.h \
 tests/../src/core/mod_inspect.h tests/../src/core/pp20.h \
 tests/../src/editor/sampler_capture.h tests/../src/editor/sampler.h \
 src/core/pattern.h src/core/project.h src/core/document.h \
 src/core/slices.h src/core/pcm.h src/core/svx.h src/core/raw.h \
 tests/../src/editor/../core/capture.h tests/../src/core/amigus_capture.h \
 tests/../src/core/capture_session.h \
 tests/../src/core/amigus_reservation.h \
 tests/../src/core/amigus_interrupt_owner.h \
 tests/../src/editor/editor_capture.h tests/../src/editor/editor.h \
 src/core/playback.h tests/../src/editor/song.h src/core/render.h \
 src/core/timeline.h src/core/flow.h src/core/frame_clock.h \
 src/core/voice.h src/core/recent.h \
 tests/../src/editor/../core/amigus_capture.h
editor_capture.o: src/editor/editor_capture.c src/editor/editor_capture.h \
 src/editor/editor.h src/core/pattern.h src/core/project.h \
 src/core/channels.h src/core/pcm.h src/core/playback.h \
 src/editor/sampler.h src/core/document.h src/core/mod_inspect.h \
 src/core/pp20.h src/core/slices.h src/core/svx.h src/core/raw.h \
 src/editor/song.h src/core/render.h src/core/timeline.h src/core/flow.h \
 src/core/frame_clock.h src/core/voice.h src/core/recent.h \
 src/editor/../core/amigus_capture.h src/editor/../core/capture_session.h \
 src/editor/../core/capture.h src/editor/../core/document.h \
 src/editor/../core/amigus_reservation.h src/editor/sampler_capture.h \
 src/editor/../core/capture.h
amigus_capture.o: src/core/amigus_capture.c src/core/amigus_capture.h \
 src/core/capture_session.h src/core/capture.h src/core/document.h \
 src/core/project.h src/core/channels.h src/core/pcm.h \
 src/core/mod_inspect.h src/core/pp20.h src/core/amigus_reservation.h
amigus_reservation.o: src/core/amigus_reservation.c \
 src/core/amigus_reservation.h
amigus_interrupt_owner.o: src/core/amigus_interrupt_owner.c \
 src/core/amigus_interrupt_owner.h src/core/amigus_reservation.h
capture_session.o: src/core/capture_session.c src/core/capture_session.h \
 src/core/capture.h src/core/document.h src/core/project.h \
 src/core/channels.h src/core/pcm.h src/core/mod_inspect.h \
 src/core/pp20.h
capture.o: src/core/capture.c src/core/capture.h src/core/document.h \
 src/core/project.h src/core/channels.h src/core/pcm.h \
 src/core/mod_inspect.h src/core/pp20.h
sampler_capture.o: src/editor/sampler_capture.c \
 src/editor/sampler_capture.h src/editor/sampler.h src/core/pattern.h \
 src/core/project.h src/core/channels.h src/core/pcm.h \
 src/core/document.h src/core/mod_inspect.h src/core/pp20.h \
 src/core/slices.h src/core/svx.h src/core/raw.h \
 src/editor/../core/capture.h src/editor/../core/document.h
sampler.o: src/editor/sampler.c src/editor/sampler.h src/core/pattern.h \
 src/core/project.h src/core/channels.h src/core/pcm.h \
 src/core/document.h src/core/mod_inspect.h src/core/pp20.h \
 src/core/slices.h src/core/svx.h src/core/raw.h \
 src/editor/sampler_internal.h src/editor/../core/pcm_internal.h \
 src/editor/../core/pcm.h src/core/wav.h
slots.o: src/editor/slots.c src/editor/sampler.h src/core/pattern.h \
 src/core/project.h src/core/channels.h src/core/pcm.h \
 src/core/document.h src/core/mod_inspect.h src/core/pp20.h \
 src/core/slices.h src/core/svx.h src/core/raw.h src/core/mod_project.h
pcm_filtered.o: src/core/pcm_filtered.c src/core/pcm.h \
 src/core/sinc_kernel.h
slices.o: src/core/slices.c src/core/slices.h src/core/pcm.h
pattern.o: src/core/pattern.c src/core/pattern.h src/core/project.h \
 src/core/channels.h src/core/pcm.h
document.o: src/core/document.c src/core/document.h src/core/project.h \
 src/core/channels.h src/core/pcm.h src/core/mod_inspect.h \
 src/core/pp20.h src/core/mod_project.h
pp20.o: src/core/pp20.c src/core/pp20.h
project.o: src/core/project.c src/core/project.h src/core/channels.h \
 src/core/pcm.h
mod_project.o: src/core/mod_project.c src/core/mod_project.h \
 src/core/project.h src/core/channels.h src/core/pcm.h \
 src/core/mod_inspect.h
mod_inspect.o: src/core/mod_inspect.c src/core/mod_inspect.h
channels.o: src/core/channels.c src/core/channels.h
pcm.o: src/core/pcm.c src/core/pcm_internal.h src/core/pcm.h
wav.o: src/core/wav.c src/core/wav.h src/core/pcm.h
svx.o: src/core/svx.c src/core/svx.h src/core/pcm.h
raw.o: src/core/raw.c src/core/raw.h src/core/pcm.h
editor.o: src/editor/editor.c src/editor/editor.h src/core/pattern.h \
 src/core/project.h src/core/channels.h src/core/pcm.h \
 src/core/playback.h src/editor/sampler.h src/core/document.h \
 src/core/mod_inspect.h src/core/pp20.h src/core/slices.h src/core/svx.h \
 src/core/raw.h src/editor/song.h src/core/render.h src/core/timeline.h \
 src/core/flow.h src/core/frame_clock.h src/core/voice.h \
 src/core/recent.h
song.o: src/editor/song.c src/editor/song.h src/core/document.h \
 src/core/project.h src/core/channels.h src/core/pcm.h \
 src/core/mod_inspect.h src/core/pp20.h src/core/pattern.h
