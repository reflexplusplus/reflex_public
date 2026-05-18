#include "resources.h"




//
//view.allow_init_animations

const Reflex::UInt64 kConfig_view_allow_init_animations[1] =
{
	0x0000000000000001ull, 
};

const Reflex::Bootstrap::EnumerableEmbeddedResource Config::view_allow_init_animations(K32("Config"), K32("view.allow_init_animations"), reinterpret_cast<const Reflex::UInt8*>(&kConfig_view_allow_init_animations), 1u, 0u);




//
//styles.txt

const Reflex::UInt64 kCustomDrawing_styles_txt[40] =
{
	0x464028f200000163ull, 0x65646f4320746e6full, 0x746170090a7b0a3aull, 0x65723a2f22203a68ull, 0x6f6d2f4544493a73ull, 0x73090a0a3b226f6eull, 0x3b3831203a657a69ull, 0xf1000e0a0a3b7d0aull, 
	0x3438332c32313511ull, 0x6f635f67620a0a3bull, 0x3b30203a72756f6cull, 0x616c707369440a0aull, 0x646e6572b4005379ull, 0x0d3b3478203a7265ull, 0x203a6461705f7000ull, 0x3920005905001136ull, 
	0x6f6c4640c0006632ull, 0x6469772032337461ull, 0x40090a3b3261008aull, 0x6c696620e1005a43ull, 0x302c353532203a6cull, 0x676210f1003b302cull, 0x68706172475b203aull, 0x6172646e6f286369ull, 
	0x6178656826203a77ull, 0xf100245d296e6f67ull, 0x206574617453401aull, 0x090a3a7265766f68ull, 0x6e61727409090a7bull, 0x203a6e6f69746973ull, 0x09090a3b35322e30ull, 0x06004d0f004e090aull, 
	0x6628747865542cb1ull, 0x0cf1012e3a746e6full, 0x66697473756a203bull, 0x65746e6563203a79ull, 0x65756c6176203b72ull, 0x3b29b0000826203aull, 0x0a3b7d0a3b7d090aull, 0x000000000000000aull, 
	
};

const Reflex::Bootstrap::EnumerableEmbeddedResource CustomDrawing::styles_txt(K32("CustomDrawing"), K32("styles.txt"), reinterpret_cast<const Reflex::UInt8*>(&kCustomDrawing_styles_txt), 313u, 355u);




