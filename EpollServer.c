/*
	main / listener
	set up server
	bind to port
	accept connection
		add client to list
		create io worker

io worker
	receive connection
	add to epoll
	epoll_wait call
		create processing worker

processing worker
	if client disconnects
		remove client from list
	wait for input 
	echo input to client
	write send / recv results
*/