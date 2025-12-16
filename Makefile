#Specify own fltk-config directory here if required
#FLTK := ../../fltk/build/

FLTK_CONFIG := fltk-config
APPLICATION := fltk-schnapsen

cxxflags = -Wall -Wextra -std=c++20
cxxflags += -g
ifeq ($(wildcard miniaudio.h),)
else
cxxflags += -DUSE_MINIAUDIO
endif

$(APPLICATION): $(APPLICATION).cxx
	$(FLTK)$(FLTK_CONFIG) --use-images --compile $(APPLICATION).cxx $(cxxflags)
#	g++ -o $(APPLICATION) -fsanitize=address `$(FLTK)$(FLTK_CONFIG) --use-images --cxxflags` $(cxxflags) $(APPLICATION).cxx `$(FLTK)$(FLTK_CONFIG) --use-images --ldflags` -static-libasan

clean:
	rm $(APPLICATION)

fetch-miniaudio:
	wget https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h
