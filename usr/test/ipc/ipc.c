/*-
 * Copyright (c) 2005, Kohsuke Ohtani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * ipc.c - IPC test program.
 */

#include <sys/prex.h>
#include <ipc/ipc.h>
#include <stdio.h>

static char stack[1024];

/*
 * Run specified thread
 */
static int
thread_run(void (*start)(void), void* stack)
{
    thread_t t;
    int error;

    error = thread_create(task_self(), &t);
    if (error)
        return error;

    error = thread_load(t, start, stack);
    if (error)
        return error;

    error = thread_resume(t);
    if (error)
        return error;

    return 0;
}

/*
 * Send thread
 */
static void
send_thread(void)
{
    struct msg msg;
    object_t o1, o2;
    int error;

    printf("Send thread is starting...\n");

    /*
	 * Find objects.
	 */
    error = object_lookup("test-A", &o1);
    error = object_lookup("test-B", &o2);

    /*
	 * Wait a sec.
	 */
    timer_sleep(1000, 0);

    /*
	 * Delete object A.
	 */
    printf("Delete object A\n");
    object_destroy(o1);

    /*
	 * Wait a sec.
	 */
    timer_sleep(1000, 0);

    /*
	 * Send message to object B.
	 */
    printf("Send message to object B.\n");
    msg_send(o2, &msg, sizeof(msg));

    printf("Send completed.\n");
    for (;;)
        ;
}

int main(int argc, char* argv[])
{
    object_t o1, o2, o3;
    struct msg msg;
    int error;

    printf("IPC test program\n");

    /*
	 * Create two objects.
	 */
    error = object_create("test-A", &o1);
    error = object_create("test-B", &o2);

    /*
	 * Create existing object.
	 * This must be error.
	 */
    error = object_create("test-B", &o3);
    if (error == 0)
        panic("Oops! object exist...");

    /*
	 * Start sender thread.
	 */
    error = thread_run(send_thread, stack + 1024);
    if (error)
        panic("failed to run thread");

    /*
	 * Wait message from non-existing object
	 * This must be error.
	 */
    o3 = 0x12345678;
    error = msg_receive(o3, &msg, sizeof(msg));
    if (error == 0)
        panic("Oops! invalid object...");

    /*
	 * Wait message from object 'A'. However, it will be failed
	 * because the sender thread will delete the object A.
	 */
    printf("Wait message from object A\n");
    error = msg_receive(o1, &msg, sizeof(msg));
    if (error)
        printf("Receive error!\n");
    else {
        printf("Rreceive ok!\n");
        msg_reply(o1, &msg, sizeof(msg));
    }

    /*
	 * Wait message from object 'B'.
	 */
    printf("Wait message from object B\n");
    error = msg_receive(o2, &msg, sizeof(msg));
    if (error)
        printf("Receive error!\n");
    else {
        printf("Receive ok!\n");
        msg_reply(o2, &msg, sizeof(msg));
    }

    printf("Test completed...\n");
    return 0;
}
