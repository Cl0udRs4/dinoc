// Add this to the server_start function in server.c
// After creating the TCP listener and before starting it:

printf("TCP listener created, now starting it\n");
fflush(stdout);

// After starting the TCP listener:
printf("TCP listener started successfully\n");
fflush(stdout);
