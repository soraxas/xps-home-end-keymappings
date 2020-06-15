NAME=caps2esc

install: pkg

pkg:
	makepkg -i --noextract

clean:
	rm -f src/$(NAME)
	rm -f $(NAME)-*.pkg.tar.xz
	rm -f -r pkg
