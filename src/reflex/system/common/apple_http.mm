#include "http.h"
#include "apple_utils.hpp"

REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

struct AppleHttp : public HttpConnection
{
	AppleHttp(CString && urlbase);
	virtual ~AppleHttp();

	void SetTimeout(Float connect_timeout_seconds, Float read_timeout_seconds) override;

	Response Request(const CString::View & method, const CString::View & resource, const ArrayView < Pair <CString> > & headers, const ArrayView <UInt8> & body, const ReceiveHeaderFn &, const ReceiveDataFn &) override;


	const CString m_url_base;
	Float m_connect_timeout = kDefaultConnectionTimeout;
	Float m_read_timeout = kDefaultReadTimeout;
};

class DispatchSemaphore
{
public:

	REFLEX_NONCOPYABLE(DispatchSemaphore);

	DispatchSemaphore(Int32 initialCount = 0) { m_dispatch_semaphore = dispatch_semaphore_create(initialCount); }
	~DispatchSemaphore() { REFLEX_DISPATCH_RELEASE(m_dispatch_semaphore); }

	void Signal() { dispatch_semaphore_signal(m_dispatch_semaphore); }
	void Wait() { dispatch_semaphore_wait(m_dispatch_semaphore, DISPATCH_TIME_FOREVER); }
	bool Wait(Float seconds)
	{
		REFLEX_ASSERT(seconds >= 0.0f);
		return dispatch_semaphore_wait(m_dispatch_semaphore, dispatch_time(DISPATCH_TIME_NOW, (int64_t)(seconds * 1e9))) == 0;
	}


private:

	dispatch_semaphore_t _Nonnull m_dispatch_semaphore;
};

REFLEX_END_INTERNAL

///
/// MEMO: https://stackoverflow.com/a/24182291
/// NSURLConnection will work with chunked encoding, but has non-disclosed internal behaviour such that it will buffer first 512 bytes before it opens the connection and let anything through. […] The solution is to make the content-type to be text/json, or send moer than 512 bytes per chunk.

@interface NS_HttpDelegate : NSObject <NSURLSessionDataDelegate> {
@public
	const Reflex::System::HttpConnection::ReceiveHeaderFn* m_preceiveheaders;
	const Reflex::System::HttpConnection::ReceiveDataFn* m_preceivedata;
	Reflex::System::ObjCRef<NSURLSessionDataTask*> m_taskRef;
	Reflex::System::ObjCRef<NSError*> m_errorRef;
	Reflex::System::ObjCRef<NSTimer*> m_connectTimer;
	NSInteger m_responseCode;
	Reflex::System::Common::DispatchSemaphore m_finished_semaphore;
	//std::atomic<bool> m_finished;
}

@end

@implementation NS_HttpDelegate
@end

@implementation NS_HttpDelegate(NSURLSessionDataDelegate)

- (void)runTaskWithConnectTimeout:(float)connect_timeout readTimeout:(float)read_timeout {
	if (connect_timeout < read_timeout) {
		m_connectTimer = Reflex::System::MakeObjCRef([NSTimer timerWithTimeInterval:connect_timeout repeats:NO block:^(NSTimer* timer) {
			[(NSURLSessionDataTask*)self->m_taskRef cancel];
		}]);
		[[NSRunLoop mainRunLoop] addTimer:m_connectTimer forMode:NSDefaultRunLoopMode];
	}

	[(NSURLSessionDataTask*)m_taskRef resume];
}

- (void)cancelConnectionTimer {
	[(NSTimer*)m_connectTimer invalidate];
	m_connectTimer.Clear();
}

- (void)URLSession:(NSURLSession* _Nonnull)session dataTask:(NSURLSessionDataTask* _Nonnull)dataTask didReceiveResponse:(NSURLResponse *_Nonnull)response completionHandler:(void (^ _Nonnull)(NSURLSessionResponseDisposition))completionHandler {
	[self cancelConnectionTimer];

	if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
		auto httpResponse = (NSHTTPURLResponse *)response;

		m_responseCode = httpResponse.statusCode;

		auto responseHeaders = [httpResponse allHeaderFields];
		for (NSString* key in responseHeaders) {
			(*m_preceiveheaders)([key UTF8String], [[responseHeaders objectForKey:key] UTF8String]);
		}

		completionHandler(NSURLSessionResponseAllow);
	}
	else {
		completionHandler(NSURLSessionResponseCancel);
	}
}

/// Disallow redirects
- (void)URLSession:(NSURLSession * _Nonnull)session task:(NSURLSessionTask * _Nonnull)task willPerformHTTPRedirection:(NSHTTPURLResponse * _Nonnull)response newRequest:(NSURLRequest * _Nonnull)request completionHandler:(void (^ _Nonnull)(NSURLRequest * _Nullable))completionHandler {
	[self cancelConnectionTimer];
	if (completionHandler) completionHandler(nil);
}

- (void)URLSession:(NSURLSession * _Nonnull)session dataTask:(NSURLSessionDataTask * _Nonnull)dataTask didReceiveData:(NSData * _Nonnull)data {
	(*m_preceivedata)({ (Reflex::UInt8*)[data bytes], (Reflex::UInt32)[data length] });
}

- (void)URLSession:(NSURLSession * _Nonnull)session task:(NSURLSessionTask * _Nonnull)task didCompleteWithError:(NSError * _Nullable)error {
	[self cancelConnectionTimer];
	m_errorRef = Reflex::System::MakeObjCRef(error);
	m_taskRef.Clear();

//	auto & finished = self->m_finished;
//	REFLEX_ATOMIC_WRITE(finished, true);
//	finished.notify_one();
	self->m_finished_semaphore.Signal();
}

@end

REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

AppleHttp::AppleHttp(CString && urlbase)
	: m_url_base(std::move(urlbase))
{}

AppleHttp::~AppleHttp() {
	// MEMO: we don't close the connections, would require keeping track of all sessions, which can only be shared by calls if no configuration (such as timeout) changes between calls).
}

void AppleHttp::SetTimeout(Float connect_timeout_seconds, Float read_timeout_seconds) {
	if (connect_timeout_seconds > read_timeout_seconds) {
		DEV_WARN("System::Http: connection timeout cannot be bigger than read timeout.");
	}

	m_connect_timeout = Min(connect_timeout_seconds, read_timeout_seconds);
	m_read_timeout = read_timeout_seconds;
}

HttpConnection::Response AppleHttp::Request(const CString::View& method, const CString::View& resource, const ArrayView<Pair<CString>>& headers, const ArrayView<UInt8>& body, const ReceiveHeaderFn& receive_headers, const ReceiveDataFn& receive_data) {
	// FIXME: [Florian] Which timeout is right?
	// Let's create a session each time for now

	auto config = [NSURLSessionConfiguration defaultSessionConfiguration];
	// Per data transfer and for the initial connection
	config.timeoutIntervalForRequest = m_read_timeout;
	// To recover the total resource
	config.timeoutIntervalForResource = 0;
	config.allowsCellularAccess = YES;

	auto delegateRef = MakeOwnedObjCRef([NS_HttpDelegate new]);
	NS_HttpDelegate* delegate = delegateRef;

	delegate->m_preceiveheaders = &receive_headers;
	delegate->m_preceivedata = &receive_data;

	delegate->m_responseCode = Reflex::System::HttpConnection::kResponseNoConnection;

	auto sessionRef = MakeObjCRef([NSURLSession sessionWithConfiguration:config delegate:delegate delegateQueue:[NSOperationQueue currentQueue]]);
	NSURLSession* session = sessionRef;

	auto urlRef = MakeObjCRef([NSURL URLWithString:ToNSString(Join(m_url_base, resource))]);
	NSURL* url = urlRef;

	auto requestRef = MakeObjCRef([NSMutableURLRequest requestWithURL:url cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:m_read_timeout]);
	NSMutableURLRequest* request = requestRef;

	request.HTTPMethod = ToNSString(method);

	for (auto & [key,value] : headers) [request setValue:ToNSString(value) forHTTPHeaderField:ToNSString(key)];

	if (body) {
		auto postDataRef = MakeObjCRef([NSData dataWithBytes:body.data length:body.size]);
		[request setHTTPBody:postDataRef];
	}

	delegate->m_taskRef = MakeObjCRef([session dataTaskWithRequest:request]);

	//REFLEX_ATOMIC_WRITE(delegate->m_finished, false);	//last for barrier

	[delegate runTaskWithConnectTimeout:m_connect_timeout readTimeout:m_read_timeout];

	#if REFLEX_INCLUDE_UI
	//delegate->m_finished.wait(false);
	delegate->m_finished_semaphore.Wait();
	#else	//Console build
	//while (!delegate->m_finished.load()) {
	while (delegate->m_finished_semaphore.Wait(0.05)) {
		[[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
	}
	#endif

	if (NSError* error = delegate->m_errorRef) {
		DEV_WARN("HttpConnection error", [error.description UTF8String]);
		return kResponseNoConnection;
	}
	else {
		return (Response)delegate->m_responseCode;
	}
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::HttpConnection> Reflex::System::HttpConnection::Create(bool https, const CString::View & url, UInt16 port) {
	return REFLEX_CREATE(Common::AppleHttp, Common::MakeUrlBase(https, url, port));
}
