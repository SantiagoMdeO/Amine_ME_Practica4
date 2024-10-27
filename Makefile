all: example_by_teach mercator_message

mercator: example_by_teach.c
	gcc example_by_teach.c -o example -lm
mercator_message: mercator_message.c
	gcc mercator_message.c -o  mercator_message -lm
