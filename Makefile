#Specify own fltk-config directory here if required
#FLTK := ../../fltk/build/

FLTK_CONFIG := fltk-config
APPLICATION := fltk-schnapsen

$(APPLICATION): $(APPLICATION).cxx
	$(FLTK)$(FLTK_CONFIG) --use-images --compile $(APPLICATION).cxx -Wall -Wextra -g -std=c++20
#	g++ -o $(APPLICATION) -fsanitize=address `$(FLTK)$(FLTK_CONFIG) --use-images --cxxflags` -Wall -g -std=c++20 $(APPLICATION).cxx `$(FLTK)$(FLTK_CONFIG) --use-images --ldflags` -static-libasan

clean:
	rm $(APPLICATION)
