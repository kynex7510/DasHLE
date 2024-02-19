# Imports

## C

### Std

malloc
btowc
wcrtomb
wcscoll
wcsftime
wcslen
wcsxfrm
snprintf
sprintf
atoi
atol
fprintf
fputc
fputs
fread
free
fseek
fstat
ftell
ftime
fwrite
fclose
fopen
printf
putc
puts
putwc
qsort
memchr
memcmp
memcpy
memmem
memmove
memrchr
memset
abort
calloc
fgets
strcasecmp
strcat
strchr
strcmp
strcoll
strcpy
strdup
strerror
strerror_r
strftime
strlcat
strlen
strncasecmp
strncmp
strncpy
strnlen
strpbrk
strrchr
strstr
strtod
strtok
strtok_r
strtol
strtoll
strtoul
wctob
wctype
wmemchr
wmemcmp
wmemcpy
wmemmove
wmemset
vfprintf
vprintf
vsnprintf
vsprintf

### Math

asin
asinf
atan
atan2
atan2f
log
log10
logf
tan
tanf
acos
acosf
tanh
ceil
ceilf
cos
cosf
cosh
floor
floorf
fmod
fmodf
exp
expf
pow
powf
sin
sinf
sinh
round
roundf
sqrt
sqrtf

### Libc

__stack_chk_guard
__stack_chk_fail

`extern unsigned long __stack_chk_guard;`
`void __stack_chk_fail(void);`

## Linux

### System

mmap
munmap
chdir
chmod
clock
clock_gettime
close
closedir
closelog
arc4random
exit
fork
getcwd
getdtablesize
getegid
getenv
geteuid
getgid
getpid
getppid
getpwuid
write
writev
gettimeofday
kill
syscall

### DL

dladdr
dlclose
dlerror
dlopen
dlsym

### PThread

pthread_attr_init
pthread_cond_broadcast
pthread_cond_destroy
pthread_cond_init
pthread_cond_signal
pthread_cond_wait
pthread_create
pthread_detach
pthread_exit
pthread_getspecific
pthread_join
pthread_key_create
pthread_key_delete
pthread_mutex_destroy
pthread_mutex_init
pthread_mutex_lock
pthread_mutex_unlock
pthread_once
pthread_setspecific
sem_destroy
sem_init
sem_post
sem_wait

## Networking

getaddrinfo
accept
connect
listen
recv
recvfrom
inet_ntop
inet_pton
bind 
gethostbyaddr
gethostbyname
gethostname
socket
socketpair
freeaddrinfo
setsockopt
getsockname
getsockopt
send
sendto
getnameinfo
shutdown
gai_strerror

`int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);`

`int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res);`

`int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);`

`int listen(int sockfd, int backlog);`

`int socket(int domain, int type, int protocol);`

`const char *gai_strerror(int errcode);`

`int inet_pton(int af, const char* src, void* dst);`

`const char *inet_ntop(int af, const void* src, char* dst, socklen_t size);`

## ZLib

crc32
deflate
deflateEnd
deflateInit2_
deflateInit_
deflateParams
deflateReset
inflate
inflateCopy
inflateEnd
inflateInit2_
inflateInit_
inflateReset
inflateSync
zError
zlibVersion
gzclose
gzopen
gzread
uncompress

## OpenGL

glActiveTexture
glAttachShader
glBindAttribLocation
glBindBuffer
glBindFramebuffer
glBindRenderbuffer
glBindTexture
glBlendEquation
glBlendFunc
glBufferData
glBufferSubData
glCheckFramebufferStatus
glClear
glClearColor
glClearDepthf
glClearStencil
glCompileShader
glCompressedTexImage2D
glCreateProgram
glCreateShader
glDeleteBuffers
glDeleteFramebuffers
glDeleteProgram
glDeleteRenderbuffers
glDeleteShader
glDeleteTextures
glDepthFunc
glDepthMask
glDisable
glDisableVertexAttribArray
glDrawArrays
glDrawElements
glEnable
glEnableVertexAttribArray
glFramebufferRenderbuffer
glFramebufferTexture2D
glGenBuffers
glGenFramebuffers
glGenRenderbuffers
glGenTextures
glGenerateMipmap
glGetBooleanv
glGetError
glGetFloatv
glGetIntegerv
glGetProgramInfoLog
glGetProgramiv
glGetShaderInfoLog
glGetShaderSource
glGetShaderiv
glGetString
glGetUniformLocation
glIsEnabled
glLineWidth
glLinkProgram
glPixelStorei
glReadPixels
glRenderbufferStorage
glScissor
glShaderSource
glStencilFunc
glStencilMask
glStencilOp
glTexImage2D
glTexParameteri
glUniform1f
glUniform1i
glUniform2f
glUniform2fv
glUniform2i
glUniform2iv
glUniform3f
glUniform3fv
glUniform3i
glUniform3iv
glUniform4f
glUniform4fv
glUniform4i
glUniform4iv
glUniformMatrix3fv
glUniformMatrix4fv
glUseProgram
glVertexAttribPointer
glViewport

## FMOD

FMOD_System_Create
_ZN4FMOD14ChannelControl11setCallbackEPF11FMOD_RESULTP19FMOD_CHANNELCONTROL24FMOD_CHANNELCONTROL_TYPE33FMOD_CHANNELCONTROL_CALLBACK_TYPEPvS6_E
_ZN4FMOD14ChannelControl4stopEv
_ZN4FMOD14ChannelControl6getDSPEiPPNS_3DSPE
_ZN4FMOD14ChannelControl7setModeEj
_ZN4FMOD14ChannelControl9getPausedEPb
_ZN4FMOD14ChannelControl9getVolumeEPf
_ZN4FMOD14ChannelControl9isPlayingEPb
_ZN4FMOD14ChannelControl9setPausedEb
_ZN4FMOD14ChannelControl9setVolumeEf
_ZN4FMOD3DSP15getMeteringInfoEP22FMOD_DSP_METERING_INFOS2_
_ZN4FMOD3DSP18setMeteringEnabledEbb
_ZN4FMOD5Sound7releaseEv
_ZN4FMOD6System10getVersionEPj
_ZN4FMOD6System11createSoundEPKcjP22FMOD_CREATESOUNDEXINFOPPNS_5SoundE
_ZN4FMOD6System11mixerResumeEv
_ZN4FMOD6System12createStreamEPKcjP22FMOD_CREATESOUNDEXINFOPPNS_5SoundE
_ZN4FMOD6System12mixerSuspendEv
_ZN4FMOD6System17getSoftwareFormatEPiP16FMOD_SPEAKERMODES1_
_ZN4FMOD6System17setSoftwareFormatEi16FMOD_SPEAKERMODEi
_ZN4FMOD6System19getStreamBufferSizeEPjS1_
_ZN4FMOD6System19setStreamBufferSizeEjj
_ZN4FMOD6System4initEijPv
_ZN4FMOD6System5closeEv
_ZN4FMOD6System6updateEv
_ZN4FMOD6System7releaseEv
_ZN4FMOD6System9playSoundEPNS_5SoundEPNS_12ChannelGroupEbPPNS_7ChannelE
_ZN4FMOD6System9setOutputE15FMOD_OUTPUTTYPE
_ZN4FMOD7Channel11getPositionEPjj
_ZN4FMOD7Channel11setPositionEjj

## Misc

__android_log_print
__assert2
__cxa_atexit
__cxa_finalize
__errno
__fpclassifyd
__gnu_Unwind_Find_exidx
__sF
_ctype_
_tolower_tab_
_toupper_tab_
access
alarm
basename
bsd_signal
bsearch
dup2
execl
fcntl
fdopen
fflush
fileno
freopen
frexp
getc
getopt
getpeername
getservbyname
getuid
getwc
gmtime
gmtime_r
if_nametoindex
initgroups
ioctl
iswctype
ldexp
localtime
longjmp
lrand48
lseek
mbrtowc
mkdir
modf
open
opendir
openlog
optarg
optind
pause
perror
pipe
poll
raise
read
readdir
realloc
remove
rename
setgid
setjmp
setlocale
setsid
setuid
setvbuf
sigaction
siglongjmp
sigprocmask
sigsetjmp
srand48
sscanf
stat
strxfrm
swprintf
syslog
time
tolower
towlower
towupper
umask
ungetc
ungetwc
unlink
usleep
waitpid