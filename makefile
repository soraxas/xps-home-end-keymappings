SRC=src/xps-keymapping.c
NAME=xps-home-end-keymappings

install: pkg

pkg: $(SRC)
	makepkg -i --noextract

clean:
	rm -f src/$(NAME)
	rm -f $(NAME)-*.pkg.tar.xz
	rm -f -r pkg
