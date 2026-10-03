capture_test.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  tests/capture_test.c tests/../src/editor/sampler_capture.h \
  tests/../src/editor/sampler.h src/core/pattern.h src/core/project.h \
  src/core/channels.h src/core/pcm.h src/core/document.h \
  src/core/mod_inspect.h src/core/pp20.h src/core/slices.h \
  src/core/svx.h src/core/raw.h tests/../src/editor/../core/capture.h \
  tests/../src/editor/../core/document.h
capture.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/capture.c src/core/capture.h src/core/document.h \
  src/core/project.h src/core/channels.h src/core/pcm.h \
  src/core/mod_inspect.h src/core/pp20.h
sampler_capture.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/editor/sampler_capture.c src/editor/sampler_capture.h \
  src/editor/sampler.h src/core/pattern.h src/core/project.h \
  src/core/channels.h src/core/pcm.h src/core/document.h \
  src/core/mod_inspect.h src/core/pp20.h src/core/slices.h \
  src/core/svx.h src/core/raw.h src/editor/../core/capture.h \
  src/editor/../core/document.h
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
pattern.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/pattern.c src/core/pattern.h src/core/project.h \
  src/core/channels.h src/core/pcm.h
document.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/document.c src/core/document.h src/core/project.h \
  src/core/channels.h src/core/pcm.h src/core/mod_inspect.h \
  src/core/pp20.h src/core/mod_project.h
pp20.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/pp20.c src/core/pp20.h
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
wav.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/wav.c src/core/wav.h src/core/pcm.h
svx.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/svx.c src/core/svx.h src/core/pcm.h
raw.o: \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/SDKSettings.json \
  src/core/raw.c src/core/raw.h src/core/pcm.h
