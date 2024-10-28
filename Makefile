all: mercator mercator_message

mercator: mercator.c
	gcc mercator.c -o example_by_teach -lm
mercator_message: mercator_message.c
	gcc mercator_message.c -o  mercator_message -lm
