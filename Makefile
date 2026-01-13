#Specify own fltk-config directory here if required
#FLTK := ../../fltk/build/

FLTK_CONFIG := fltk-config
APPLICATION := fltk-schnapsen

cxxflags = -Isrc -Iinclude -Wall -Wextra -std=c++20
cxxflags += -g
ifeq ($(wildcard include/miniaudio.h),)
else
cxxflags += -DUSE_MINIAUDIO
endif

$(APPLICATION): $(APPLICATION).cxx include/debug.h \
                                   include/messages.h \
                                   include/Rect.h \
                                   include/UI.h \
                                   include/CardImage.h src/CardImage.cxx \
                                   include/Card.h src/Card.cxx \
                                   include/Cards.h src/Cards.cxx \
                                   include/Util.h src/Util.cxx \
                                   include/Deck.h src/Deck.cxx \
                                   include/Engine.h src/Engine.cxx \
                                   include/Welcome.h src/Welcome.cxx \
                                   include/Unittest.h src/Unittest.cxx

	$(FLTK)$(FLTK_CONFIG) --use-images --compile $(APPLICATION).cxx $(cxxflags)
#	g++ -o $(APPLICATION) -fsanitize=address `$(FLTK)$(FLTK_CONFIG) --use-images --cxxflags` $(cxxflags) $(APPLICATION).cxx `$(FLTK)$(FLTK_CONFIG) --use-images --ldflags` -static-libasan

clean:
	rm $(APPLICATION)

fetch-miniaudio:
	wget -P include/ https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h

cppcheck:
	cppcheck --std=c++20 --max-configs=4 --enable=all --disable=missingInclude --disable=information --check-level=exhaustive $(APPLICATION).cxx
