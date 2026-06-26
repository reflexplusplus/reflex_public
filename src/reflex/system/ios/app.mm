#include "app.h"




#ifdef REFLEX_SYSTEM_AUDIO
#include "../common/instance/audioapp.cpp"
#include "app/audio_app.mm"
#else
#include "../common/instance/app.cpp"
#endif

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application willFinishLaunchingWithOptions:(NSDictionary *)launchOptions 
{
	REFLEX_USE(Reflex);

	[NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationWillTerminate:) name:UIApplicationWillTerminateNotification object:nil];

	root_module.Init();
	
	auto app = REFLEX_CREATE(AppType);

	app->Initialise<false>({});

	return YES;
}

- (void)applicationWillTerminate:(NSNotification*)notification 
{
	REFLEX_USE(Reflex);

	if (auto app = AppType::Get()) 
	{
		app->Deinitialise();

		root_module.Deinit();
	}
}

- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options {
//current impl: use SceneDelegate defined in plis
	return [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role];

//future
//	auto config = [[UISceneConfiguration alloc] initWithName:nil sessionRole:connectingSceneSession.role];
//
//	config.delegateClass = NSClassFromString(@"ReflexSceneDelegate");
//
//	return config;
}

- (void)application:(UIApplication *)application didDiscardSceneSessions:(NSSet<UISceneSession *> *)sceneSessions {}

@end

const Reflex::System::EnvironmentType Reflex::System::kEnvironmentType = kEnvironmentTypeMobileApp;

int main(int argc, char* argv[]) {
	NSString* appDelegateClassName;
	@autoreleasepool {
		// Setup code that might create autoreleased objects goes here.
		appDelegateClassName = NSStringFromClass([AppDelegate class]);
	}
	return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}

void Reflex::System::App::Quit() {
//	auto self = iOS::TheLibrary::Get();
//	auto vc = (self ? self->GetViewController() : nil);
//	dispatch_async(dispatch_get_main_queue(), ^{
//		[vc quitApp];
//	});
}
