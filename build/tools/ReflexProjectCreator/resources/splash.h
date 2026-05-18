#resource "splash.glx" as gSplashStyles;

void RunReflexAnimation(GLX::Object where)
{
	using GLX;

	Animation Init(Animation a, Object target, Float32 time, Key32 easing)
	{
		a.SetTarget(target);

		a.SetTime(time);
		
		cast@InterpolatedAnimation(a).SetEasing(easing);

		return a;
	}
	
	auto stroke = gSplashStyles#Stroke;
	
	auto character = gSplashStyles#Character;
	
	Point offset = { 32.0f, 0.0f };
		
	auto container = Init(new, gSplashStyles#Container);
	
	auto sequence = new Sequence;
	
	auto multi = new Multi;
	
	auto time = 0.65f;

	auto opacity = 0.5f;
	
	foreach (i : ["|", "|", "|", "R", "E", "F", "L", "E", "X"])
	{
		auto char = new Object;
		
		switch (@Key32 i)
		{
			case ('|') Init(char, stroke, i);
			default: Init(char, character, i);
		}
	
		char.SetOpacity(#fixed, opacity);
		
		char.SetOpacity(#animation, 0.0f);
			
		multi.Add
		(
			Init(CreateAnimation('Property', {#id: #offset, #from: offset, #to: null Point}), char, time, #ease_out_cubic),
			Init(CreateAnimation('Opacity', {#id: #animation, #from: 0.0f, #to: 1.0f}), char, time, #ease_in)
		);
		
		opacity = Min(opacity + 0.15f, 1.0f);
		
		container.AddInline(char);
		
		offset.x += 32.0f;
	}
	

	auto overlay = Init(new, gSplashStyles);

	overlay.EnableMouse(false, false);


	sequence.Add(multi);
	
	sequence.Add(Init(CreateAnimation(#Wait, null), overlay, 0.25f, #linear));
	
	sequence.Add(Init(CreateAnimation(#Opacity, {#from: 1.0f, #to: 0.0f}), overlay, 0.75f, #ease_out));

	sequence.Add(new Animation { [overlay](){overlay.Detach();}, null, null });
	
	
	overlay#animate = sequence;

	sequence.Play();

		
	overlay.AddFloat(container, kAlignmentCenter);

	where.AddStretch(overlay);
}	
