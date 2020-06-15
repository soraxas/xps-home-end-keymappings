NAME=caps2esc

all: $(NAME)

$(NAME):
	gcc $(NAME).c -o $(NAME) -I/usr/include/libevdev-1.0 -levdev -ludev

clean:
	rm $(NAME)
