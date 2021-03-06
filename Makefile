CC= gcc
CFLAGS= -O1
PROGRAM_NAME=admin


$(PROGRAM_NAME):
	$(CC) $(CFLAGS) main.c -o $(PROGRAM_NAME) -lcrypt

install: $(PROGRAM_NAME)
	cp ./allowed_to_run_as_root /etc/allowed_to_run_as_root
	cp ./secure_environment_vars /etc/secure_environment_vars
	cp ./$(PROGRAM_NAME) /usr/bin/$(PROGRAM_NAME)
	chown root:root /usr/bin/$(PROGRAM_NAME)
	chown root:root /etc/allowed_to_run_as_root
	chown root:root /etc/secure_environment_vars
	chmod u+s+g /usr/bin/$(PROGRAM_NAME)

uninstall:
	rm -f /etc/allowed_to_run_as_root
	rm -f /etc/secure_environment_vars
	rm -f /usr/bin/$(PROGRAM_NAME)

clean:
	rm -f *.exe *.out *.o $(PROGRAM_NAME)
