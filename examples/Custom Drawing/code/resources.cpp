#include "resources.h"




//
//view.allow_init_animations

const Reflex::UInt64 kConfig_view_allow_init_animations[1] =
{
	0x0000000000000001ull, 
};

const Reflex::File::EnumerableEmbeddedResource Config::view_allow_init_animations(K32("Config"), K32("view.allow_init_animations"), reinterpret_cast<const Reflex::UInt8*>(&kConfig_view_allow_init_animations), 1u, 0u);




//
//styles.glx

const Reflex::UInt64 kCustomDrawing_styles_glx[37] =
{
	0x464027f200000144ull, 0x65646f4320746e6full, 0x746170090a7b0a3aull, 0x7365723a22203a68ull, 0x6e6f6d2f4544493aull, 0x6973090a0a3b226full, 0x0a3b3432203a657aull, 0x11f1000e0a0a3b7dull, 
	0x3b3231352c323135ull, 0x6c6f635f67620a0aull, 0x0a3b30203a72756full, 0x79616c707369440aull, 0x65646e6572b40052ull, 0x000d3b3478203a72ull, 0x10203a6461705f60ull, 0x617274090a04f100ull, 
	0x3a6e6f697469736eull, 0x00163532312e3020ull, 0x0e30323331006002ull, 0x74616f6c4640c000ull, 0x009f646977203233ull, 0x203a676251003a02ull, 0x766e614382007543ull, 0x02f1002d29287361ull, 
	0x6f68206574617453ull, 0x0a7b090a3a726576ull, 0x2d09090a39002a09ull, 0x3a7209f000af0000ull, 0x3832312c35353220ull, 0x78655409090a2c29ull, 0x0058746e6f662874ull, 0x6c6176203b6564c1ull, 
	0xf0000826203a6575ull, 0x697473756a203b0cull, 0x746e6563203a7966ull, 0x3b7d090a3b297265ull, 0x000000000a3b7d0aull, 
};

const Reflex::File::EnumerableEmbeddedResource CustomDrawing::styles_glx(K32("CustomDrawing"), K32("styles.glx"), reinterpret_cast<const Reflex::UInt8*>(&kCustomDrawing_styles_glx), 292u, 324u);




