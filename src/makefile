.PHONY: with-cap2esc clean all

NAME=xps-keymapping
FLAGS=-I/usr/include/libevdev-1.0 -levdev -ludev -Wall -O3

all: $(NAME)

with-cap2esc: FLAGS += -DCAP2ESC
with-cap2esc: all
	
$(NAME): $(NAME).c
	gcc $(FLAGS) $(NAME).c -o $(NAME)

clean:
	rm -f $(NAME)
