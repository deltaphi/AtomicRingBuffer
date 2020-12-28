# AtomicRingBuffer
A Lock-Free Ring Buffer for single-sender-single-receiver usecases.

AtomicRingBuffer manages a section of memory as a round-robin byte-buffer. It is intended to be used in a way
that minimizes copying data - a writer can request a buffer section to write to and a reader can directly read
published data from the buffer.

Synchronization is based on a set of std::atomic<> variables. This means that operations themselves are not
synchronized with each other. If multiple operations are performed at performed concurrently, they may spuriously
fail in case of a race condition. It is guaranteed that failing operations make no modification to the buffer.
Therefore, in these cases, it is safe to simply retry the operation. This results in a busy-wait which is likely
to succeed in a few attempts.

allocate/publish and peek/consume operations must always be performed in pairs and always in the correct order.
Thus, if there are multiple readers or multiple writers, they must be synchronizied amongst each other, but writers
do not have to be synchronized with readers and vice versa.

Also note that AtomicRingBuffer is intended to be used together with memcpy(). Therefore, AtomicRingBuffer will only
provide pointers to consecutive blocks of memory. As AtomicRingBuffer also requires the user to make use of all bytes
in the buffer, this means that an allocation or a peek may be partially fulfilled. In this case, the user must manage
the fragmentation of the data across the wrap-around point of the buffer.

## Usage

Create an AtomicRingBuffer object and provide it with a pointer and size to a uint8_t buffer of your chosing.

You can allocate Buffer space using `AtomicRingBuffer::allocate()` and then write data to the allocated memory.
Once the write completes, the data must be published using `AtomicRingBuffer::publish()`. As soon as the data
has been published, it is available to the reader.

A reader first uses `AtomicRingBuffer::peek()` to obtain a pointer to published data for reading of said data.
Once the reader finishes processing the data, it must call `AtomicRingBuffer::consume()` to free the space in
the buffer for further use by the writer.


![Windows CI](https://github.com/deltaphi/AtomicRingBuffer/workflows/Windows%20CI/badge.svg)
![Linux CI](https://github.com/deltaphi/AtomicRingBuffer/workflows/Linux%20CI/badge.svg)

