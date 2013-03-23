#include <python.h>

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <winsock2.h>
//#include <
#include <io.h>
#include <process.h>
#include <conio.h>
#include <pthread.h>

#include <glib.h>
#pragma comment(lib, "glib-2.0.lib")
#pragma comment(lib, "user32.lib")
#pragma warning(disable:4996)

#if PY_VERSION_HEX < 0x03000000
#define _PyLong_FromLong PyInt_FromLong
#else
#define _PyLong_FromLong PyLong_FromLong
#endif

#define BUFFER_SIZE 4096

#ifdef _fstat
#define stat_fn _fstat
#define stat_st struct _stat
#else
#define stat_fs fstat
#define stat_st stat_
#endif

typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed char int8_t;
typedef unsigned char uint8_t;

int StartResult = -1;

void *ConnectThread(void *socket)
{
	sockaddr_in service;
	SOCKET write_socket = *((SOCKET *)socket);
	// The following 3 lines could proabbly be stuffed and summed up by a static structure that is pre-initialised..
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr("127.0.0.1"); // Always use the loopback interface..
	service.sin_port = htons(1900); // Use a nice'n'high number so as to stay out the way..
	_sleep(10); // sleep 10ms so accept() can get started..
	if (connect(write_socket, (sockaddr*)&service,  sizeof(service)) == SOCKET_ERROR)
		return (void *)-1;
	else
		return 0;
}

extern PyObject *PyPipe(PyObject *self, PyObject *)
{
	WSADATA wsaData;
	SOCKET read_socket, write_socket, AcceptSocket;
	pthread_attr_t ThreadAttr;
	pthread_t ThreadCon;
	sockaddr_in service;
	int Result;

	/*HANDLE read, write;
	int fds[2];
	BOOL res;*/
	//Py_BEGIN_ALLOW_THREADS
	if ((StartResult = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
	{
		//Py_END_ALLOW_THREADS
		PyErr_SetFromWindowsErr(StartResult);
		return Py_BuildValue("(ii)", 0, 0);
	}
	read_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (read_socket == INVALID_SOCKET)
	{
		WSACleanup();
		//Py_END_ALLOW_THREADS
		PyErr_SetString(PyExc_IOError, "Could not get a socket");
		return Py_BuildValue("(ii)", 0, 0);
	}
	else
	{
		bool opt = true;
		setsockopt(read_socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, sizeof(opt));
	}
	// The following 3 lines could proabbly be stuffed and summed up by a static structure that is pre-initialised..
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr("127.0.0.1"); // Always use the loopback interface..
	service.sin_port = htons(1900); // Use a nice'n'high number so as to stay out the way..
	if (bind(read_socket, (sockaddr *)&service, sizeof(service)) == SOCKET_ERROR)
	{
		closesocket(read_socket);
		WSACleanup();
		//Py_END_ALLOW_THREADS
		PyErr_SetString(PyExc_IOError, "Could not bind the read socket");
		return Py_BuildValue("(ii)", 0, 0);
	}
	if (listen(read_socket, 1) == SOCKET_ERROR)
	{
		closesocket(read_socket);
		WSACleanup();
		//Py_END_ALLOW_THREADS
		PyErr_SetString(PyExc_IOError, "Could not listen on the read socket");
		return Py_BuildValue("(ii)", 0, 0);
	}
	write_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (write_socket == INVALID_SOCKET)
	{
		WSACleanup();
		//Py_END_ALLOW_THREADS
		PyErr_SetString(PyExc_IOError, "Could not get a socket");
		return Py_BuildValue("(ii)", 0, 0);
	}
	else
	{
		bool opt = true;
		setsockopt(write_socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, sizeof(opt));
	}
	pthread_attr_init(&ThreadAttr);
	pthread_attr_setdetachstate(&ThreadAttr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setscope(&ThreadAttr, PTHREAD_SCOPE_PROCESS);
	pthread_create(&ThreadCon, &ThreadAttr, ConnectThread, &write_socket);
	pthread_attr_destroy(&ThreadAttr);
	AcceptSocket = accept(read_socket, NULL, NULL); // This is the socket that actually gets used for IO..
	pthread_join(ThreadCon, (void **)&Result);
	closesocket(read_socket); // nolonger needed.. replaced by AcceptSocket
	// The following two calls ensure only one-way transfer of data on these sockets.
	shutdown(AcceptSocket, SD_SEND);
	shutdown(write_socket, SD_RECEIVE);

	/*res = CreatePipe(&read, &write, NULL, 0);
	fds[0] = _open_osfhandle((intptr_t)read, 0);
	fds[1] = _open_osfhandle((intptr_t)write, 1);*/
	//Py_END_ALLOW_THREADS
	return Py_BuildValue("(ii)", AcceptSocket, write_socket); // The two sockets specified here are already endpoint connected to each-other, ready for IO.
	//return Py_BuildValue("(ii)", fds[0], fds[1]);
}

typedef struct stat stat_;

typedef enum _GIOWin32ChannelType
{
	G_IO_WIN32_WINDOWS_MESSAGES,
	G_IO_WIN32_FILE_DESC,
	G_IO_WIN32_CONSOLE,
	G_IO_WIN32_SOCKET
} GIOWin32ChannelType;

typedef struct _GIOWin32Channel
{
	GIOChannel channel;
	int fd;
	GIOWin32ChannelType type;
	HWND hwnd;
	CRITICAL_SECTION mutex;
	int direction;
	BOOL running;
	BOOL needs_close;
	UINT thread_id;
	HANDLE data_avail_event;
	WORD revents;
	BYTE *buffer;
	int wrp, rdp;
	HANDLE space_avail_event;
	int event_mask;
	int last_events;
	HANDLE event;
	BOOL write_would_have_blocked;
	BOOL ever_writable;
} GIOWin32Channel;

typedef struct _GIOWin32Watch
{
	GSource source;
	GPollFD pollfd;
	GIOChannel *channel;
	GIOCondition condition;
} GIOWin32Watch;

void io_init(GIOWin32Channel *channel)
{
	InitializeCriticalSection(&channel->mutex);
	channel->running = FALSE;
	channel->needs_close = FALSE;
	channel->thread_id = 0;
	channel->data_avail_event = NULL;
	channel->revents = 0;
	channel->buffer = NULL;
	channel->space_avail_event = NULL;
	channel->event_mask = 0;
	channel->last_events = 0;
	channel->write_would_have_blocked = FALSE;
	channel->ever_writable = FALSE;
}

void io_free(GIOChannel *channel)
{
	GIOWin32Channel *WinChan = (GIOWin32Channel *)channel;

	DeleteCriticalSection(&WinChan->mutex);
	if (WinChan->data_avail_event != NULL)
		CloseHandle(WinChan->data_avail_event);
	g_free(WinChan->buffer);
	if (WinChan->space_avail_event != NULL)
		CloseHandle(WinChan->space_avail_event);
	if (WinChan->type == G_IO_WIN32_SOCKET && WinChan->fd != -1)
		WSAEventSelect(WinChan->fd, NULL, 0);
	if (WinChan->event != FALSE)
		WSACloseEvent(WinChan->event);
	g_free(channel);
}

void create_events(GIOWin32Channel *channel)
{
	SECURITY_ATTRIBUTES sec_attrs;
	sec_attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
	sec_attrs.lpSecurityDescriptor = NULL;
	sec_attrs.bInheritHandle = FALSE;
	if ((channel->data_avail_event = CreateEvent(&sec_attrs, TRUE, FALSE, NULL)) == 0 ||
		(channel->space_avail_event = CreateEvent(&sec_attrs, FALSE, FALSE, NULL)) == 0)
	{
		char *msg = g_win32_error_message(GetLastError());
		g_error("Error creating event: %s", msg);
		g_free(msg);
	}
}

void create_thread(GIOWin32Channel *channel, GIOCondition condition, unsigned int (__stdcall *thread)(void *parameter))
{
	HANDLE thread_handle = (HANDLE)_beginthreadex(NULL, 0, thread, channel, 0, &channel->thread_id);
	if (thread_handle == 0)
		g_warning("Error creating thread: %s.", g_strerror(errno));
	else if (CloseHandle(thread_handle) == 0)
	{
		char *msg = g_win32_error_message(GetLastError());
		g_warning("Error closing thread handle: %s.", msg);
		g_free(msg);
	}
	WaitForSingleObject(channel->space_avail_event, INFINITE);
}

unsigned int __stdcall read_thread(void *parameter)
{
	GIOWin32Channel *channel = (GIOWin32Channel *)parameter;
	BYTE *buffer;
	int nBytes;
	g_io_channel_ref((GIOChannel *)channel);

	channel->direction = 0;
	channel->buffer = (BYTE *)g_malloc(BUFFER_SIZE);
	channel->rdp = channel->wrp = 0;
	channel->running = TRUE;
	SetEvent(channel->space_avail_event);
	EnterCriticalSection(&channel->mutex);
	while (channel->running != FALSE)
	{
		if ((channel->wrp + 1) % BUFFER_SIZE == channel->rdp)
		{
			ResetEvent(channel->space_avail_event);
			LeaveCriticalSection(&channel->mutex);
			WaitForSingleObject(channel->space_avail_event, INFINITE);
			EnterCriticalSection(&channel->mutex);
		}
		buffer = channel->buffer + channel->wrp;
		nBytes = MIN((channel->rdp + BUFFER_SIZE - channel->wrp - 1) % BUFFER_SIZE, BUFFER_SIZE - channel->wrp);
		LeaveCriticalSection(&channel->mutex);
		nBytes = read(channel->fd, buffer, nBytes);
		EnterCriticalSection(&channel->mutex);
		channel->revents = G_IO_IN;
		if (nBytes == 0)
			channel->revents |= G_IO_HUP;
		else if (nBytes < 0)
			channel->revents |= G_IO_ERR;
		if (nBytes <= 0)
			break;
		channel->wrp = (channel->wrp + nBytes) % BUFFER_SIZE;
		SetEvent(channel->data_avail_event);
	}
	channel->running = FALSE;
	if (channel->needs_close != FALSE)
	{
		close(channel->fd);
		channel->fd = -1;
	}
	SetEvent(channel->data_avail_event);
	LeaveCriticalSection(&channel->mutex);
	g_io_channel_unref((GIOChannel *)channel);
	return 0;
}

unsigned int __stdcall write_thread(void *parameter)
{
	GIOWin32Channel *channel = (GIOWin32Channel *)parameter;
	BYTE *buffer;
	int nBytes;
	g_io_channel_ref((GIOChannel *)channel);

	channel->direction = 1;
	channel->buffer = (BYTE *)g_malloc(BUFFER_SIZE);
	channel->rdp = channel->wrp = 0;
	channel->running = TRUE;
	SetEvent(channel->space_avail_event);
	EnterCriticalSection(&channel->mutex);
	while (channel->running != 0 || channel->rdp != channel->wrp)
	{
		if (channel->wrp == channel->rdp)
		{
			ResetEvent(channel->space_avail_event);
			channel->revents = G_IO_OUT;
			SetEvent(channel->data_avail_event);
			LeaveCriticalSection(&channel->mutex);
			WaitForSingleObject(channel->space_avail_event, INFINITE);
			EnterCriticalSection(&channel->mutex);
			if (channel->wrp == channel->rdp)
				break;
		}
		buffer = channel->buffer + channel->rdp;
		if (channel->rdp < channel->wrp)
			nBytes = channel->wrp - channel->rdp;
		else
			nBytes = BUFFER_SIZE - channel->rdp;
		LeaveCriticalSection(&channel->mutex);
		nBytes = write(channel->fd, buffer, nBytes);
		EnterCriticalSection(&channel->mutex);
		channel->revents = 0;
		if (nBytes > 0)
			channel->revents |= G_IO_OUT;
		else if (nBytes <= 0)
			channel->revents |= G_IO_ERR;
		channel->rdp = int((channel->rdp + nBytes) % BUFFER_SIZE);
		if (nBytes <= 0)
			break;
		SetEvent(channel->data_avail_event);
	}
	channel->running = FALSE;
	if (channel->needs_close != FALSE)
	{
		close(channel->fd);
		channel->fd = -1;
	}
	LeaveCriticalSection(&channel->mutex);
	g_io_channel_unref((GIOChannel *)channel);
	return 0;
}

GIOStatus buffer_read(GIOWin32Channel *channel, char *dest, gsize count, gsize *bytes_read, GError **err)
{
	gsize nBytes, left = count;
	EnterCriticalSection(&channel->mutex);
	if (channel->wrp == channel->rdp)
	{
		LeaveCriticalSection(&channel->mutex);
		WaitForSingleObject(channel->data_avail_event, INFINITE);
		EnterCriticalSection(&channel->mutex);
		if (channel->wrp == channel->rdp && channel->running == FALSE)
		{
			LeaveCriticalSection(&channel->mutex);
			*bytes_read = 0;
			return G_IO_STATUS_EOF;
		}
	}
	if (channel->rdp < channel->wrp)
		nBytes = channel->wrp - channel->rdp;
	else
		nBytes = BUFFER_SIZE - channel->rdp;
	LeaveCriticalSection(&channel->mutex);
	nBytes = MIN(left, nBytes);
	memcpy(dest, channel->buffer + channel->rdp, nBytes);
	dest += nBytes;
	left -= nBytes;
	EnterCriticalSection(&channel->mutex);
	channel->rdp = int((channel->rdp + nBytes) % BUFFER_SIZE);
	SetEvent(channel->space_avail_event);
	if (channel->running != FALSE && channel->wrp == channel->rdp)
		ResetEvent(channel->data_avail_event);
	LeaveCriticalSection(&channel->mutex);
	*bytes_read = count - left;
	return (*bytes_read > 0 ? G_IO_STATUS_NORMAL : G_IO_STATUS_EOF);
}

GIOStatus buffer_write(GIOWin32Channel *channel, const char *dest, gsize count, gsize *bytes_written, GError **err)
{
	gsize nBytes, left = count;
	EnterCriticalSection(&channel->mutex);
	if ((channel->wrp + 1) % BUFFER_SIZE == channel->rdp)
	{
		ResetEvent(channel->data_avail_event);
		LeaveCriticalSection(&channel->mutex);
		WaitForSingleObject(channel->data_avail_event, INFINITE);
		EnterCriticalSection(&channel->mutex);
	}
	nBytes = MIN((channel->rdp + BUFFER_SIZE - channel->wrp - 1) % BUFFER_SIZE, BUFFER_SIZE - channel->wrp);
	LeaveCriticalSection(&channel->mutex);
	nBytes = MIN(left, nBytes);
	memcpy(channel->buffer + channel->wrp, dest, nBytes);
	dest += nBytes;
	left -= nBytes;
	EnterCriticalSection(&channel->mutex);
	channel->wrp = int((channel->wrp + nBytes) % BUFFER_SIZE);
	SetEvent(channel->space_avail_event);
	if ((channel->wrp + 1) % BUFFER_SIZE == channel->rdp)
		ResetEvent(channel->data_avail_event);
	LeaveCriticalSection(&channel->mutex);
	*bytes_written = count - left;
	return (*bytes_written > 0 ? G_IO_STATUS_NORMAL : G_IO_STATUS_EOF);
}

static GIOStatus io_read(GIOChannel *channel, char *buf, gsize count, gsize *bytes_read, GError **err)
{
	ssize_t result;
	GIOWin32Channel *WinChan = (GIOWin32Channel *)channel;

	if (WinChan->thread_id != 0)
		return buffer_read(WinChan, buf, count, bytes_read, err);

	result = read(WinChan->fd, buf, (uint32_t)count);
	if (result < 0)
	{
		*bytes_read = 0;
		switch (errno)
		{
#ifdef EAGAIN
			case EAGAIN:
				return G_IO_STATUS_AGAIN;
#endif
			default:
				g_set_error_literal(err, G_IO_CHANNEL_ERROR, g_io_channel_error_from_errno(errno), g_strerror(errno));
				return G_IO_STATUS_ERROR;
		}
	}

	*bytes_read = result;
	return (result > 0 ? G_IO_STATUS_NORMAL : G_IO_STATUS_EOF);
}

static GIOStatus io_write(GIOChannel *channel, const char *buf, gsize count, gsize *bytes_written, GError **err)
{
	ssize_t result;
	GIOWin32Channel *WinChan = (GIOWin32Channel *)channel;

	if (WinChan->thread_id != 0)
		return buffer_write(WinChan, buf, count, bytes_written, err);

	result = write(WinChan->fd, buf, (uint32_t)count);
	if (result < 0)
	{
		*bytes_written = 0;
		switch (errno)
		{
#ifdef EAGAIN
			case EAGAIN:
			{
				return G_IO_STATUS_AGAIN;
			}
#endif
			default:
			{
				g_set_error_literal(err, G_IO_CHANNEL_ERROR, g_io_channel_error_from_errno(errno), g_strerror(errno));
				return G_IO_STATUS_ERROR;
			}
		}
	}

	*bytes_written = result;
	return G_IO_STATUS_NORMAL;
}

static GIOStatus io_unimpl_set_flags(GIOChannel *channel, GIOFlags flags, GError **err)
{
	GIOWin32Channel *WinChan = (GIOWin32Channel *)channel;
	g_set_error_literal(err, G_IO_CHANNEL_ERROR, G_IO_CHANNEL_ERROR_FAILED, "Not implemented on Win32");
	return G_IO_STATUS_ERROR;
}

static BOOL io_prepare(GSource *source, int *timeout)
{
	GIOWin32Watch *watch = (GIOWin32Watch *)source;
	GIOCondition condition = g_io_channel_get_buffer_condition(watch->channel);
	GIOWin32Channel *channel = (GIOWin32Channel *)watch->channel;
	*timeout = -1;
	switch (channel->type)
	{
		case G_IO_WIN32_WINDOWS_MESSAGES:
		case G_IO_WIN32_CONSOLE:
			break;
		case G_IO_WIN32_FILE_DESC:
		{
			EnterCriticalSection(&channel->mutex);
			if (channel->running != FALSE)
			{
				if (channel->direction == 0 && channel->wrp == channel->rdp)
					channel->revents = 0;
			}
			else
			{
				if (channel->direction == 1 && (channel->wrp + 1) % BUFFER_SIZE == channel->rdp)
					channel->revents = 0;
			}
			LeaveCriticalSection(&channel->mutex);
			break;
		}
		case G_IO_WIN32_SOCKET:
		{
			int event_mask = 0;
			if ((watch->condition & G_IO_IN) != FALSE)
				event_mask |= (FD_READ | FD_ACCEPT);
			if ((watch->condition & G_IO_OUT) != FALSE)
				event_mask |= (FD_WRITE | FD_CONNECT);
			event_mask |= FD_CLOSE;
			if (channel->event_mask != event_mask)
			{
				WSAEventSelect(channel->fd, (HANDLE)watch->pollfd.fd, event_mask);
				channel->event_mask = event_mask;
				channel->last_events = 0;
				if (((event_mask & FD_WRITE) != FALSE) && channel->ever_writable != FALSE && channel->write_would_have_blocked == FALSE)
					WSASetEvent((WSAEVENT)watch->pollfd.fd);
			}
			break;
		}
		default:
		{
			g_assert_not_reached();
			abort();
		}
	}
	return (watch->condition & condition) == watch->condition;
}

static BOOL io_check(GSource *source)
{
	MSG msg;
	GIOWin32Watch *watch = (GIOWin32Watch *)source;
	GIOWin32Channel *channel = (GIOWin32Channel *)watch->channel;
	GIOCondition condition = g_io_channel_get_buffer_condition(watch->channel);

	switch (channel->type)
	{
		case G_IO_WIN32_WINDOWS_MESSAGES:
			return PeekMessage(&msg, channel->hwnd, 0, 0, PM_NOREMOVE);
		case G_IO_WIN32_FILE_DESC:
		{
			watch->pollfd.revents = (watch->pollfd.events & channel->revents);
			return ((watch->pollfd.revents | condition) & watch->condition);
		}
		case G_IO_WIN32_CONSOLE:
		{
			if (watch->channel->is_writeable != FALSE)
				return TRUE;
			else if (watch->channel->is_readable != FALSE)
			{
				INPUT_RECORD buffer;
				DWORD bytes;
				if (PeekConsoleInput((HANDLE)watch->pollfd.fd, &buffer, 1, &bytes) != 0 && bytes == 1)
				{
					if (_kbhit() != 0)	
						return TRUE;
					ReadConsoleInput((HANDLE)watch->pollfd.fd, &buffer, 1, &bytes);
				}
			}
			return FALSE;
		}
		case G_IO_WIN32_SOCKET:
		{
			WSANETWORKEVENTS events;
			if ((channel->last_events & FD_WRITE) == FALSE)
			{
				WSAEnumNetworkEvents(channel->fd, 0, &events);
				if (watch->pollfd.revents != 0 && events.lNetworkEvents == 0 && (channel->event_mask & FD_WRITE) == FALSE)
				{
					channel->event_mask = 0;
					WSAEventSelect(channel->fd, (HANDLE)watch->pollfd.fd, 0);
					ResetEvent((HANDLE) watch->pollfd.fd);
				}
				else if ((events.lNetworkEvents & FD_WRITE) != FALSE)
					channel->ever_writable = TRUE;
				channel->last_events = events.lNetworkEvents;
			}
			watch->pollfd.revents = 0;
			if ((channel->last_events & (FD_READ | FD_ACCEPT)) != FALSE)
				watch->pollfd.revents |= G_IO_IN;
			if ((channel->last_events & FD_WRITE) != FALSE)
				watch->pollfd.revents |= G_IO_OUT;
			else
			{
				if ((events.lNetworkEvents & FD_CONNECT) != FALSE)
				{
					if (events.iErrorCode[FD_CONNECT_BIT] == 0)
						watch->pollfd.revents |= G_IO_OUT;
					else
						watch->pollfd.revents |= (G_IO_HUP | G_IO_ERR);
				}
				if (watch->pollfd.revents == 0 && ((channel->last_events & (FD_CLOSE))) != FALSE)
					watch->pollfd.revents |= G_IO_HUP;
			}
			if ((watch->pollfd.revents & G_IO_HUP) == FALSE && channel->ever_writable != FALSE &&
				channel->write_would_have_blocked == FALSE && (channel->event_mask & FD_WRITE) != FALSE)
				watch->pollfd.revents |= G_IO_OUT;
			return ((watch->pollfd.revents | condition) & watch->condition);
		}
		default:
		{
			g_assert_not_reached();
			abort();
		}
	}
	return FALSE;
}

static BOOL io_dispatch(GSource *source, GSourceFunc callback, void *user_data)
{
	GIOFunc func = (GIOFunc)callback;
	GIOWin32Watch *watch = (GIOWin32Watch *)source;
	GIOWin32Channel *channel = (GIOWin32Channel *)watch->channel;
	GIOCondition condition = g_io_channel_get_buffer_condition(watch->channel);

	if (func == NULL)
	{
		g_warning("IO Watch dispateched without callback\nYou must call g_source_connect().");
		return FALSE;
	}
	return (*func)(watch->channel, (GIOCondition)((watch->pollfd.revents | condition) & watch->condition), user_data);
}

static void io_finalize(GSource *source)
{
	GIOWin32Watch *watch = (GIOWin32Watch *)source;
	GIOWin32Channel *channel = (GIOWin32Channel *)watch->channel;

	switch (channel->type)
	{
		case G_IO_WIN32_WINDOWS_MESSAGES:
		case G_IO_WIN32_CONSOLE:
		case G_IO_WIN32_FILE_DESC:
		case G_IO_WIN32_SOCKET:
			break;
		default:
		{
			g_assert_not_reached();
			abort();
		}
	}
	g_io_channel_unref(watch->channel);
}

GSourceFuncs io_watch_funcs =
{
	io_prepare,
	io_check,
	io_dispatch,
	io_finalize
};

#pragma warning(disable : 4244)

static GIOStatus io_fd_seek(GIOChannel *channel, __int64 offset, GSeekType type, GError **err)
{
	GIOWin32Channel *WinChan = (GIOWin32Channel *)channel;
	int wence;
	off_t result;

	switch (type)
	{
		case G_SEEK_SET:
		{
			wence = SEEK_SET;
			break;
		}
		case G_SEEK_CUR:
		{
			wence = SEEK_CUR;
			break;
		}
		case G_SEEK_END:
		{
			wence = SEEK_END;
			break;
		}
		default:
		{
			g_assert_not_reached();
			abort();
		}
	}

	result = offset;
	if (result != offset)
	{
		g_set_error_literal(err, G_IO_CHANNEL_ERROR, g_io_channel_error_from_errno(EINVAL), g_strerror(EINVAL));
		return G_IO_STATUS_ERROR;
	}

	result = lseek(WinChan->fd, result, wence);

	if (result < 0)
	{
		g_set_error_literal(err, G_IO_CHANNEL_ERROR, g_io_channel_error_from_errno(errno), g_strerror(errno));
		return G_IO_STATUS_ERROR;
	}
	return G_IO_STATUS_NORMAL;
}

#pragma warning(default : 4244)

static GIOStatus io_fd_close(GIOChannel *channel, GError **err)
{
	GIOWin32Channel *WinChan = (GIOWin32Channel *)channel;
	EnterCriticalSection(&WinChan->mutex);
	if (WinChan->running != FALSE)
	{
		WinChan->running = FALSE;
		WinChan->needs_close = TRUE;
		if (WinChan->direction == 0)
			SetEvent(WinChan->data_avail_event);
		else
			SetEvent(WinChan->space_avail_event);
	}
	else
	{
		close(WinChan->fd);
		WinChan->fd = -1;
	}
	LeaveCriticalSection(&WinChan->mutex);
	return G_IO_STATUS_NORMAL;
}

static GSource *io_fd_create_watch(GIOChannel *channel, GIOCondition condition)
{
	GIOWin32Channel *WinChan = (GIOWin32Channel *)channel;
	GSource *source = g_source_new(&io_watch_funcs, sizeof(GIOWin32Watch));
	GIOWin32Watch *watch = (GIOWin32Watch *)source;

	watch->channel = channel;
	g_io_channel_ref(channel);
	watch->condition = condition;

	if (WinChan->data_avail_event == NULL)
		create_events(WinChan);
	watch->pollfd.fd = (intptr_t)WinChan->data_avail_event;
	watch->pollfd.events = condition;

	EnterCriticalSection(&WinChan->mutex);
	if (WinChan->thread_id == 0)
	{
		if ((condition & G_IO_IN) != 0)
			create_thread(WinChan, condition, read_thread);
		else if ((condition & G_IO_OUT) != 0)
			create_thread(WinChan, condition, write_thread);
	}
	g_source_add_poll(source, &watch->pollfd);
	LeaveCriticalSection(&WinChan->mutex);
	return source;
}

static GIOStatus io_console_close(GIOChannel *channel, GError **err)
{
	GIOWin32Channel *WinChan = (GIOWin32Channel *)channel;
	if (close(WinChan->fd) < 0)
	{
		g_set_error_literal(err, G_IO_CHANNEL_ERROR, g_io_channel_error_from_errno(errno), g_strerror(errno));
		return G_IO_STATUS_ERROR;
	}
	return G_IO_STATUS_NORMAL;
}

static GSource *io_console_create_watch(GIOChannel *channel, GIOCondition condition)
{
	GIOWin32Channel *WinChan = (GIOWin32Channel *)channel;
	GSource *source = g_source_new(&io_watch_funcs, sizeof(GIOWin32Watch));
	GIOWin32Watch *watch = (GIOWin32Watch *)source;

	watch->channel = channel;
	g_io_channel_ref(channel);
	watch->condition = condition;

	watch->pollfd.fd = _get_osfhandle(WinChan->fd);
	watch->pollfd.events = condition;
	g_source_add_poll(source, &watch->pollfd);
	return source;
}

static GIOStatus io_sock_read(GIOChannel *channel, char *buf, gsize count, gsize *bytes_read, GError **err)
{
	GIOWin32Channel *WinChan = (GIOWin32Channel *)channel;
	int winsock_error;
	ssize_t result;
	GIOChannelError error;

	result = recv(WinChan->fd, buf, (uint32_t)count, 0);
	if (result == SOCKET_ERROR)
	{
		char *emsg;
		winsock_error = WSAGetLastError();
		emsg = g_win32_error_message(winsock_error);
		*bytes_read = 0;
		switch (winsock_error)
		{
			case WSAEINVAL:
			{
				error = G_IO_CHANNEL_ERROR_INVAL;
				break;
			}
			case WSAEWOULDBLOCK:
			{
				g_free(emsg);
				return G_IO_STATUS_AGAIN;
			}
			default:
			{
				error = G_IO_CHANNEL_ERROR_FAILED;
				break;
			}
		}
		g_set_error_literal(err, G_IO_CHANNEL_ERROR, error, emsg);
		g_free(emsg);
		return G_IO_STATUS_ERROR;
	}
	else
	{
		*bytes_read = result;
		if (result == 0)
			return G_IO_STATUS_EOF;
		return G_IO_STATUS_NORMAL;
	}
}

static GIOStatus io_sock_write(GIOChannel *channel, const char *buf, gsize count, gsize *bytes_written, GError **err)
{
	GIOWin32Channel *WinChan = (GIOWin32Channel *)channel;
	int winsock_error;
	ssize_t result;
	GIOChannelError error;

	result = send(WinChan->fd, buf, (uint32_t)count, 0);
	if (result == SOCKET_ERROR)
	{
		char *emsg;
		winsock_error = WSAGetLastError();
		emsg = g_win32_error_message(winsock_error);
		*bytes_written = 0;
		switch (winsock_error)
		{
			case WSAEINVAL:
			{
				error = G_IO_CHANNEL_ERROR_INVAL;
				break;
			}
			case WSAEWOULDBLOCK:
			{
				WinChan->write_would_have_blocked = TRUE;
				WinChan->last_events = 0;
				g_free(emsg);
				return G_IO_STATUS_AGAIN;
			}
			default:
			{
				error = G_IO_CHANNEL_ERROR_FAILED;
				break;
			}
		}
		g_set_error_literal(err, G_IO_CHANNEL_ERROR, error, emsg);
		g_free(emsg);
		return G_IO_STATUS_ERROR;
	}
	else
	{
		*bytes_written = result;
		WinChan->write_would_have_blocked = FALSE;
		return G_IO_STATUS_NORMAL;
	}
}

static GIOStatus io_sock_close(GIOChannel *channel, GError **err)
{
	GIOWin32Channel *WinChan = (GIOWin32Channel *)channel;
	if (WinChan->fd != -1)
	{
		closesocket(WinChan->fd);
		WinChan->fd = -1;
	}
	fprintf(stdout, "fract4dc|win32func.cpp:Error, io_sock_close called!\n");
	return G_IO_STATUS_NORMAL;
}

static GSource *io_sock_create_watch(GIOChannel *channel, GIOCondition condition)
{
	GIOWin32Channel *WinChan = (GIOWin32Channel *)channel;
	GSource *source = g_source_new(&io_watch_funcs, sizeof(GIOWin32Watch));
	GIOWin32Watch *watch = (GIOWin32Watch *)source;

	watch->channel = channel;
	g_io_channel_ref(channel);
	watch->condition = condition;

	if (WinChan->event == WSA_INVALID_EVENT)
		WinChan->event = WSACreateEvent();

	watch->pollfd.fd = (gintptr)WinChan->event;
	watch->pollfd.events = condition;
	g_source_add_poll(source, &watch->pollfd);
	return source;
}

static GIOFlags get_fd_flags(GIOChannel *Channel, stat_st *s)
{
	HANDLE h = (HANDLE)_get_osfhandle(((GIOWin32Channel *)Channel)->fd);
	char c;
	DWORD count;

	if ((s->st_mode & _S_IFIFO) != 0)
	{
		Channel->is_readable = (PeekNamedPipe(h, &c, 0, &count, NULL, NULL) != 0 || GetLastError() == ERROR_BROKEN_PIPE);
		Channel->is_seekable = FALSE;
	}
	else
	{
		Channel->is_readable = ReadFile(h, &c, 0, &count, NULL) != 0;
		Channel->is_seekable = TRUE;
	}
	Channel->is_writeable = WriteFile(h, &c, 0, &count, NULL) != 0;

	return (GIOFlags)0;
}

static GIOFlags get_console_flags(GIOChannel *Channel)
{
	HANDLE h = (HANDLE)_get_osfhandle(((GIOWin32Channel *)Channel)->fd);
	char c;
	DWORD count;
	INPUT_RECORD record;

	Channel->is_readable = PeekConsoleInput(h, &record, 1, &count);
	Channel->is_writeable = WriteFile(h, &c, 0, &count, NULL);
	Channel->is_seekable = FALSE;

	return (GIOFlags)0;
}

static GIOFlags get_fd_flags_(GIOChannel *Channel)
{
	stat_st s;
	stat_fn(((GIOWin32Channel *)Channel)->fd, &s);
	return get_fd_flags(Channel, &s);
}

static GIOStatus set_sock_flags(GIOChannel *Channel, GIOFlags flags, GError **err)
{
	GIOWin32Channel *WinChan = (GIOWin32Channel *)Channel;
	unsigned long arg;
	if ((flags & G_IO_FLAG_NONBLOCK) != FALSE)
	{
		arg = 1;
		if (ioctlsocket(WinChan->fd, FIONBIO, &arg) == SOCKET_ERROR)
		{
			char *emsg = g_win32_error_message(WSAGetLastError());
			g_set_error_literal(err, G_IO_CHANNEL_ERROR, G_IO_CHANNEL_ERROR_FAILED, emsg);
			g_free(emsg);
			return G_IO_STATUS_ERROR;
		}
	}
	else
	{
		arg = 0;
		if (ioctlsocket(WinChan->fd, FIONBIO, &arg) == SOCKET_ERROR)
		{
			char *emsg = g_win32_error_message(WSAGetLastError());
			g_set_error_literal(err, G_IO_CHANNEL_ERROR, G_IO_CHANNEL_ERROR_FAILED, emsg);
			g_free(emsg);
			return G_IO_STATUS_ERROR;
		}
	}
	return G_IO_STATUS_NORMAL;
}

static GIOFlags get_sock_flags(GIOChannel *Channel)
{
	return (GIOFlags)0;
}

GIOFuncs win32_channel_fd_funcs =
{
	io_read,
	io_write,
	io_fd_seek,
	io_fd_close,
	io_fd_create_watch,
	io_free,
	io_unimpl_set_flags,
	get_fd_flags_,
};

GIOFuncs win32_channel_console_funcs =
{
	io_read,
	io_write,
	NULL,
	io_console_close,
	io_console_create_watch,
	io_free,
	io_unimpl_set_flags,
	get_console_flags,
};

GIOFuncs win32_channel_sock_funcs =
{
	io_sock_read,
	io_sock_write,
	NULL,
	io_sock_close,
	io_sock_create_watch,
	io_free,
	set_sock_flags,
	get_sock_flags,
};

GIOChannel *new_io_channel_fd(int fd)
{
	GIOWin32Channel *Channel = g_new(GIOWin32Channel, 1);
	GIOChannel *ret = (GIOChannel *)Channel;
	stat_st s;

	stat_fn(fd, &s);
	g_io_channel_init(ret);
	io_init(Channel);

	Channel->fd = fd;

	if ((s.st_mode & _S_IFCHR) != 0)
	{
		ret->funcs = &win32_channel_console_funcs;
		Channel->type = G_IO_WIN32_CONSOLE;
		get_console_flags(ret);
	}
	else
	{
		ret->funcs = &win32_channel_fd_funcs;
		Channel->type = G_IO_WIN32_FILE_DESC;
		get_fd_flags(ret, &s);
	}

	return ret;
}

GIOChannel *new_io_channel_socket(int socket)
{
	GIOWin32Channel *Channel = g_new(GIOWin32Channel, 1);
	GIOChannel *ret = (GIOChannel *)Channel;

	g_io_channel_init(ret);
	io_init(Channel);

	ret->funcs = &win32_channel_sock_funcs;
	Channel->type = G_IO_WIN32_SOCKET;
	Channel->fd = socket;
	Channel->event = WSA_INVALID_EVENT;

	ret->is_readable = TRUE;
	ret->is_writeable = TRUE;
	ret->is_seekable = FALSE;

	return ret;
}

void doNotCrash(const wchar_t *, const wchar_t *, const wchar_t *, unsigned int, UINT_PTR)
{
	return;
}

inline bool isFD(int fd)
{
	bool ret;
	_invalid_parameter_handler old_handler;
	// The next line turns off crashing for the line after the next comment..
	old_handler = _set_invalid_parameter_handler(doNotCrash);
	// Taken from the working VC++6 version of the CRT file fstat.c
	ret = ((GetFileType((HANDLE)_get_osfhandle(fd)) & ~FILE_TYPE_REMOTE) != 0);
	// ..and set it back again..
	_set_invalid_parameter_handler(old_handler);
	return ret;
}

inline bool isSocket(int socket)
{
	bool ret;
	int optval;
	static int optlen = sizeof(optval);
	ret = (getsockopt(socket, SOL_SOCKET, SO_TYPE, (char *)&optval, &optlen) != SOCKET_ERROR);
	return ret;
}

GIOChannel *new_io_channel(int fd)
{
	bool is_fd = isFD(fd), is_socket = isSocket(fd);

	if (is_fd == true && is_socket == true)
		g_warning ("new_io_channel: %d is both a file descriptor and a socket. File descriptor interpretation assumed.", fd);

	if (is_fd == true)
		return new_io_channel_fd(fd);
	else if (is_socket == true)
		return new_io_channel_socket(fd);

	g_warning("new_io_channel: %d is neither a file descriptor or a socket.", fd);
	return NULL;
}

UINT add_watch(GIOChannel *Channel, GIOCondition condition, GIOFunc func, void *user_data)
{
	UINT id;
	GSource *Source = g_io_create_watch(Channel, condition);
	g_source_set_callback(Source, (GSourceFunc)func, user_data, NULL);
	id = g_source_attach(Source, NULL);
	g_source_unref(Source);
	return id;
}

static BOOL iowatch_marshal(GIOChannel *source, GIOCondition condition, void *user_data)
{
	PyGILState_STATE state;
	PyObject *tuple, *func, *args, *ret;
	BOOL res;

	g_return_val_if_fail(user_data != NULL, FALSE);

	state = PyGILState_Ensure();
	tuple = (PyObject *)user_data;
	func = PyTuple_GetItem(tuple, 0);

	args = Py_BuildValue("(Oi)", PyTuple_GetItem(tuple, 1), condition);

	ret = PyObject_CallObject(func, args);
	Py_DECREF(args);

	if (ret == NULL)
	{
		PyErr_Print();
		res = FALSE;
	}
	else
	{
		if (ret == Py_None)
		{
			if (PyErr_Warn(PyExc_Warning, "io_add_watch callback returned None; should return True/False") != 0)
				PyErr_Print();
		}
		res = PyObject_IsTrue(ret);
		Py_DECREF(ret);
	}

	PyGILState_Release(state);
	return res;
}

extern PyObject *Py_io_add_watch(PyObject *self, PyObject *args)
{
	PyObject *callback, *data;
	int fd, condition;
	UINT handler_id;
	Py_ssize_t len;
	GIOChannel *IOChannel;

	len = PyTuple_Size(args);
	if (len < 3 || len > 3)
	{
		PyErr_SetString(PyExc_TypeError, "io_add_watch takes just 3 arguments");
		return NULL;
	}
	if (PyArg_ParseTuple(args, "iiO:io_add_watch", &fd, &condition, &callback) == 0)
		return NULL;

	if (fd < 0)
		return NULL;
	if (PyCallable_Check(callback) == 0)
	{
		PyErr_SetString(PyExc_TypeError, "third argument not a function");
		return NULL;
	}

	data = Py_BuildValue("(Oi)", callback, fd);
	if (data == NULL)
		return NULL;

	IOChannel = new_io_channel(fd);
	handler_id = add_watch(IOChannel, (GIOCondition)condition, iowatch_marshal, data);
	// The following call should be equivilant to the above one.
	//handler_id = g_io_add_watch(IOChannel, (GIOCondition)condition, iowatch_marshal, data);
	g_io_channel_unref(IOChannel);

	return _PyLong_FromLong(handler_id);
}

extern PyObject *PyRead(PyObject *self, PyObject *args)
{
	int fd, size, ret, err;
	Py_ssize_t len;
	PyObject *buffer;
	bool is_socket;
	len = PyTuple_Size(args);
	if (len < 2 || len > 2)
	{
		PyErr_SetString(PyExc_TypeError, "read takes just 2 arguments");
		return NULL;
	}
	if (PyArg_ParseTuple(args, "ii:read", &fd, &size) == 0)
		return NULL;
	if (fd < 0)
		return NULL;

	buffer = PyString_FromStringAndSize((char *)NULL, size);
	if (buffer == NULL)
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	is_socket = isSocket(fd);
	if (isFD(fd) == true)
		ret = read(fd, PyString_AsString(buffer), size);
	else if (is_socket == true)
	{
		// The following ensures no error caused by M$..
		int recved = 0;
		do
		{
			ret = recv(fd, PyString_AsString(buffer), size, 0);
			if (ret == -1)
				err = WSAGetLastError();
			else
				break;
		}
		while (err == WSAEWOULDBLOCK);
	}
	Py_END_ALLOW_THREADS
	if (ret < 0)
	{
		int errcode = (is_socket == true ? err : errno);
		char *err = (char *)malloc(snprintf(NULL, 0, "reading failed with code %u", errcode));
		sprintf(err, "reading failed with code %u", errcode);
		Py_DECREF(buffer);
		PyErr_SetString(PyExc_IOError, err);
		free(err);
		return NULL;
	}
	else if (ret == 0)
		PyErr_SetString(PyExc_IOError, "reading halted - socket has become closed!");
	if (ret != size)
		_PyString_Resize(&buffer, ret);

	return buffer;
}