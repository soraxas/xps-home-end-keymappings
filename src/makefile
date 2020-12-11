NAME=xps-keymapping

all: $(NAME)

$(NAME): $(NAME).c
	gcc $(NAME).c -o $(NAME) -I/usr/include/libevdev-1.0 -levdev -ludev -Wall -O3

clean:
	rm $(NAME)
