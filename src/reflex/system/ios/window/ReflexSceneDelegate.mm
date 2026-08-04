#include "../app.h"

@interface ReflexSceneDelegate : UIResponder <UIWindowSceneDelegate>

@property (strong, nonatomic) UIWindow * window;

@end

@interface ReflexSceneDelegate ()

- (void)scene:(UIScene *)scene continueUserActivity:(NSUserActivity *)userActivity;

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions;

- (void)handleUniversalLink:(NSUserActivity*)userActivity;

@end

@implementation ReflexSceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions {
	// Handle universal links on cold start (!= deep links ⚠️)
	[self handleUniversalLink:connectionOptions.userActivities.allObjects.firstObject];

	// If a storyboard is configured, UIKit will have already created the window
	// and set self.window before this method is called. If not, create the
	// window and view controller programmatically — otherwise the app launches
	// with no scene content and shows a blank screen.
	if (!self.window) {
		auto windowscene = (UIWindowScene *)scene;
		self.window = [[UIWindow alloc] initWithWindowScene:windowscene];
		self.window.rootViewController = Reflex::System::iOS::Window::CreateViewController();
		[self.window makeKeyAndVisible];
	}
}

- (void)sceneDidDisconnect:(UIScene *)scene {
	// Called as the scene is being released by the system.
	// This occurs shortly after the scene enters the background, or when its session is discarded.
	// Release any resources associated with this scene that can be re-created the next time the scene connects.
	// The scene may re-connect later, as its session was not necessarily discarded (see `application:didDiscardSceneSessions` instead).
}

- (void)sceneDidBecomeActive:(UIScene *)scene {
	// Called when the scene has moved from an inactive state to an active state.
	REFLEX_ASSERT(Reflex::System::iOS::Window::st_self);

	if (auto window = Reflex::System::iOS::Window::st_self) {
		window->GetClient()->OnSetFocus();
	}
}

- (void)sceneWillResignActive:(UIScene *)scene {
	// Called when the scene will move from an active state to an inactive state.
	REFLEX_ASSERT(Reflex::System::iOS::Window::st_self);

	if (auto window = Reflex::System::iOS::Window::st_self) {
		window->GetClient()->OnLoseFocus();
	}
}

- (void)sceneWillEnterForeground:(UIScene *)scene {
	// Called as the scene transitions from the background to the foreground.
}

- (void)sceneDidEnterBackground:(UIScene *)scene {
	// Called as the scene transitions from the foreground to the background.
}

- (void)scene:(UIScene *)scene continueUserActivity:(NSUserActivity *)userActivity {
	[self handleUniversalLink:userActivity];
}

- (void)handleUniversalLink:(NSUserActivity*)userActivity {
	REFLEX_USE(Reflex);
	if ([userActivity.activityType isEqualToString:NSUserActivityTypeBrowsingWeb]) {
		auto app = AppType::Get();
		REFLEX_ASSERT(app)

		auto url = New<ObjectOf<CString>>(System::ToCStringView([userActivity.webpageURL absoluteString]));
		app->GetGlobal()->SetProperty(K32("DeepLinkUrl"), url);
		return;
	}

	DEV_WARN("Deep link of type", System::ToCStringView(userActivity.activityType), "unmatched");
}

@end
