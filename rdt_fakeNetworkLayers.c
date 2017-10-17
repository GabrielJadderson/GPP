/* Fake network/upper layers for station 1
 *
 * Send 20 packets and receive 10 before we stop
 * */
void FakeNetworkLayer1()
{
	char *buffer;
	int i,j;
    long int events_we_handle;
    event_t event;
	FifoQueueEntry e;

    from_network_layer_queue = InitializeFQ();
    for_network_layer_queue = InitializeFQ();

    // Setup some messages
    for( i = 0; i < 20; i++ ) {
        buffer = (char *) malloc ( sizeof(char) * MAX_PKT);
    	sprintf( buffer, "D: %d", i );
  	    EnqueueFQ( NewFQE( (void *) buffer ), from_network_layer_queue );
    }

    events_we_handle = network_layer_allowed_to_send | data_for_network_layer;

    // Give selective repeat a chance to start
    sleep(2);

    i = 0;
    j = 0;
    while( true ) {
    	// Wait until we are allowed to transmit
    	Wait(&event, events_we_handle);
    	switch(event.type) {
    		case network_layer_allowed_to_send:
				Lock( network_layer_lock );
    			if( i < 20 && network_layer_enabled) {
        			// Signal element is ready
        			logLine(info, "Sending signal for message #%d\n", i);
        			network_layer_enabled = false;
        			Signal(network_layer_ready, NULL);
        			i++;
    			}
				Unlock( network_layer_lock );
    			break;
    		case data_for_network_layer:
				Lock( network_layer_lock );

				e = DequeueFQ( for_network_layer_queue );
    			logLine(succes, "Received message: %s\n" ,( (char *) e->val) );

				Unlock( network_layer_lock );

    			j++;
    			break;
    	}

		if( i >= 20 && j >= 10) {
		    logLine(info, "Station %d done. - (\'sleep(5)\')\n", ThisStation);
		    /* A small break, so all stations can be ready */
		    sleep(5);
		    Stop();
		}
    }
}

/* Fake network/upper layers for station 2
 *
 * Receive 20 messages, take the first 10, lowercase first letter and send to station 1.
 * With this, some acks will be piggybacked, some will be pure acks.
 *
 **/
void FakeNetworkLayer2()
{
    long int events_we_handle;
    event_t event;
	int j;
	FifoQueueEntry e;

    from_network_layer_queue = InitializeFQ();
    for_network_layer_queue = InitializeFQ();

    events_we_handle = network_layer_allowed_to_send | data_for_network_layer;

    j = 0;
    while( true ) {
    	// Wait until we are allowed to transmit
    	Wait(&event, events_we_handle);

    	switch(event.type) {
    		case network_layer_allowed_to_send:
				Lock( network_layer_lock );
    			if( network_layer_enabled && !EmptyFQ( from_network_layer_queue )  ) {
        			logLine(info, "Signal from network layer for message\n");
        			network_layer_enabled = false;
        			ClearEvent( network_layer_ready ); // Don't want to signal too many events
        			Signal(network_layer_ready, NULL);
    			}
				Unlock( network_layer_lock );
    			break;
    		case data_for_network_layer:
				Lock( network_layer_lock );

				e = DequeueFQ( for_network_layer_queue );
    			logLine(succes, "Received message: %s\n" ,( (char *) e->val) );

    			if( j < 10) {
   					( (char *) e->val)[0] = 'd';
   					EnqueueFQ( e, from_network_layer_queue );
    			}

				Unlock( network_layer_lock );


				j++;
    			logLine(info, "j: %d\n" ,j );
    			break;
    	}
		if( EmptyFQ( from_network_layer_queue ) && j >= 20) {
			logLine(succes, "Stopping - received 20 messages and sent 10\n"  );
		    sleep(5);
		    Stop();
		}

    }
}



void FakeNetworkLayer_Test1()
{
    char *buffer;
    int i,j;
    long int events_we_handle;
    event_t event;
    FifoQueueEntry e;

    from_network_layer_queue = InitializeFQ();
    for_network_layer_queue = InitializeFQ();

    // Setup some messages
    for( i = 0; i < 2; i++ ) {
        buffer = (char *) malloc ( sizeof(char) * MAX_PKT);
    	sprintf( buffer, "%d to %d", ThisStation, i );
        EnqueueFQ( NewFQE( (void *) buffer ), from_network_layer_queue );
    }

    events_we_handle = network_layer_allowed_to_send | data_for_network_layer;

    // Give selective repeat a chance to start
    sleep(2);

    i = 0;
    j = 0;
    while( true ) {
    	// Wait until we are allowed to transmit
    	Wait(&event, events_we_handle);
    	switch(event.type) {
    		case network_layer_allowed_to_send:
				Lock( network_layer_lock );
    			if( i < 2 && network_layer_enabled) {
        			// Signal element is ready
        			logLine(info, "Sending signal for message #%d\n", i);
        			network_layer_enabled = false;
        			Signal(network_layer_ready, 1);
        			i++;
    			}
				Unlock( network_layer_lock );
    			break;
    		case data_for_network_layer:
				Lock( network_layer_lock );

				e = DequeueFQ( for_network_layer_queue );
    			logLine(succes, "Received message: %s\n" ,( (char *) e->val) );

				Unlock( network_layer_lock );

    			j++;
    			break;
    	}

		if( i >= 20 && j >= 10) {
		    logLine(info, "Station %d done. - (\'sleep(5)\')\n", ThisStation);
		    // A small break, so all stations can be ready
		    sleep(5);
		    Stop();
		}
    }
}


