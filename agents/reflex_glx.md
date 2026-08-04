
##### GLX Layers

- [FX](#sym-25178416844117253)

- [Image](#sym-965297026512196869)

- [Layout](#sym-13854936138885633285)

- [Text](#sym-8974791055023019269)

- [Vector](#sym-15552789302868972805)

---

<a id="sym-23111219025157"></a>
<a id="sym-25178416844117253"></a>

#### FX


## Functions

[Blur](#sym-8972045093452125445), [Brightness](#sym-18053251530965914885), [Clip](#sym-8972197732294857989), [Contrast](#sym-13436796608486315269), [Exposure](#sym-5483585407675274501), [Grayscale](#sym-14076807402003371269), [Greyscale](#sym-17817314091039986949), [InnerShadow](#sym-248109501796848901), [Invert](#sym-13416385592455861509), [Mask](#sym-8973691161143153925), [Pixelate](#sym-158847273793492229), [Render](#sym-14882069948452050181), [Saturate](#sym-6266793163915007237), [Sepia](#sym-1015067584279614725), [Shadow](#sym-15063430568932807941), [Tint](#sym-8974808346561352965)
---

<a id="sym-965297026512196869"></a>

#### Image


## Functions

[Image](#sym-965297026512196869), [ImageFill](#sym-16429307426932528389), [ImageSelector](#sym-18413114587156059397)
---

<a id="sym-13854936138885633285"></a>

#### Layout


## Functions

[Align](#sym-924432199397152005), [Group](#sym-955949296480425221), [If](#sym-25178902175421701), [Inline](#sym-13414860187576046853), [Split](#sym-1016746787643331845), [Tile](#sym-8974807998669001989), [ViewPortGrid](#sym-9424123253560972549)
---

<a id="sym-8974791055023019269"></a>

#### Text


## Functions

[Text](#sym-8974791055023019269), [TextEdit](#sym-9244992215028077829)
---

<a id="sym-15552789302868972805"></a>

#### Vector


## Functions

[Bar](#sym-830870812209714437), [BarGraph](#sym-11434150497636848901), [Border](#sym-12244258531699594501), [Canvas](#sym-12340501112351102213), [Circle](#sym-12381779072370349317), [CircleFill](#sym-633738152695043333), [CircleGradient](#sym-15868944856299607301), [ColorCanvas](#sym-1377939582888187141), [ColourCanvas](#sym-1646520676934227205), [Fill](#sym-8972647153377744133), [Gradient](#sym-3672027794893706501), [GraphicCanvas](#sym-3111336564773360901), [Line](#sym-8973573496219112709), [Path](#sym-8974154334711321861), [Polygon](#sym-2073253062503306501), [Rotate](#sym-14933918995512825093), [Scale](#sym-1014689171891033349), [Translate](#sym-10667000374281377029), [Triangle](#sym-4954868727309931781), [TriangleFill](#sym-1689799709461320965)
---


#### API Reference


#### 

<a id="sym-924432199397152005"></a>

### Align

```cpp
void Align(Layers content, Margin& indent, Size& size, Alignment& position);
```

<a id="sym-830870812209714437"></a>

### Bar

```cpp
void Bar(Layers content, Margin& indent, Float32& min_size, Range& range, Range& region);
```

<a id="sym-11434150497636848901"></a>

### BarGraph

```cpp
void BarGraph(Margin& indent, Colour& color, Range& range, Float32& gap, Float32[]& values);
```

<a id="sym-8972045093452125445"></a>

### Blur

```cpp
void Blur(Layers content, Float32& radius, Float32& opacity, Point& offset);
```

<a id="sym-12244258531699594501"></a>

### Border

```cpp
void Border(Margin& indent, Colour& color, Margin& width, Margin& corner);
```

<a id="sym-18053251530965914885"></a>

### Brightness

```cpp
void Brightness(Layers content, Float32& amount);
```

<a id="sym-12340501112351102213"></a>

### Canvas

```cpp
void Canvas(Margin& indent, Colour& color, Key32& id);
```

<a id="sym-12381779072370349317"></a>

### Circle

```cpp
void Circle(Margin& indent, Colour& color, Range& sweep, Float32& width, bool round_cap);
```

<a id="sym-633738152695043333"></a>

### CircleFill

```cpp
void CircleFill(Margin& indent, Colour& color, Range& sweep);
```

<a id="sym-15868944856299607301"></a>

### CircleGradient

```cpp
void CircleGradient(Margin& indent, Colour& from, Colour& to, Float32 dither);
```

<a id="sym-8972197732294857989"></a>

### Clip

```cpp
void Clip(Layers content, Margin& indent);
```

<a id="sym-1377939582888187141"></a>

### ColorCanvas

```cpp
void ColorCanvas(Margin& indent, Colour& color, Key32& id);
```

<a id="sym-1646520676934227205"></a>

### ColourCanvas

```cpp
void ColourCanvas(Margin& indent, Colour& color, Key32& id);
```

<a id="sym-13436796608486315269"></a>

### Contrast

```cpp
void Contrast(Layers content, Float32& amount);
```

<a id="sym-5483585407675274501"></a>

### Exposure

```cpp
void Exposure(Layers content, Colour& rgb);
```

<a id="sym-8972647153377744133"></a>

### Fill

```cpp
void Fill(Margin& indent, Colour& color, Margin& corner);
```

<a id="sym-3672027794893706501"></a>

### Gradient

```cpp
void Gradient(Margin& indent, Colour& from, Colour& to, Float32 dither, Float32& angle);
```

<a id="sym-3111336564773360901"></a>

### GraphicCanvas

```cpp
void GraphicCanvas(Margin& indent, Colour& color, Key32& id);
```

<a id="sym-14076807402003371269"></a>

### Grayscale

```cpp
void Grayscale(Layers content, Float32& amount);
```

<a id="sym-17817314091039986949"></a>

### Greyscale

```cpp
void Greyscale(Layers content, Float32& amount);
```

<a id="sym-955949296480425221"></a>

### Group

```cpp
void Group(Layers content, Margin& indent, Size& size, Alignment& position);
```

<a id="sym-25178902175421701"></a>

### If

```cpp
void If(Margin& indent, Layers else, Layers then, Float32& input, Float32& value, Key32& op);
```

<a id="sym-965297026512196869"></a>

### Image

```cpp
void Image(Margin& indent, Colour& color, ImageSet& source, Key32& autofit, Alignment anchor, Key32& fit);
```

<a id="sym-16429307426932528389"></a>

### ImageFill

```cpp
void ImageFill(Margin& indent, Colour& color, ImageSet& source);
```

<a id="sym-18413114587156059397"></a>

### ImageSelector

```cpp
void ImageSelector(Margin& indent, Colour& color, ImageSet& source, Float32& frame, Key32& autofit, Alignment anchor);
```

<a id="sym-13414860187576046853"></a>

### Inline

```cpp
void Inline(Layers content, Margin& indent, Key32 axis);
```

Experimental
<a id="sym-248109501796848901"></a>

### InnerShadow

```cpp
void InnerShadow(Margin& indent, Colour& color, Margin& width, Float32 dither);
```

InnerShadow approximates a Gaussian blur applied to an oversized border, clipped to the bounds of the area being rendered.
```cpp
//approximation of Clip(content: Blur(radius: 8; content: Border(indent: -4; width: 8; colour: 0,64)));
fg: InnerShadow(width: 8; color: 0,64);
```

<a id="sym-13416385592455861509"></a>

### Invert

```cpp
void Invert(Layers content, Float32& amount);
```

<a id="sym-8973573496219112709"></a>

### Line

```cpp
void Line(Margin& indent, Colour& color, Alignment& position, Float32& width, Float32[]& pattern);
```

<a id="sym-8973691161143153925"></a>

### Mask

```cpp
void Mask(Layers content, Layers mask, bool& invert);
```

<a id="sym-8974154334711321861"></a>

### Path

```cpp
void Path(Margin& indent, Colour& color, bool normalized, Float32[]& points, Float32& width, bool& closed);
```

<a id="sym-158847273793492229"></a>

### Pixelate

```cpp
void Pixelate(Layers content, Size& size);
```

Experimental
<a id="sym-2073253062503306501"></a>

### Polygon

```cpp
void Polygon(Margin& indent, Colour& color, bool normalized, Float32[]& points);
```

<a id="sym-14882069948452050181"></a>

### Render

```cpp
void Render(Layers content, Margin& pad, Float32& density);
```

<a id="sym-14933918995512825093"></a>

### Rotate

```cpp
void Rotate(Layers content, Margin& indent, Float32& angle);
```

<a id="sym-6266793163915007237"></a>

### Saturate

```cpp
void Saturate(Layers content, Float32& amount);
```

<a id="sym-1014689171891033349"></a>

### Scale

```cpp
void Scale(Layers content, Size& scale, Size& origin);
```

<a id="sym-1015067584279614725"></a>

### Sepia

```cpp
void Sepia(Layers content, Float32& amount);
```

<a id="sym-15063430568932807941"></a>

### Shadow

```cpp
void Shadow(Margin& indent, Colour& color, Float32& blur, Float32& corner, Float32 dither);
```

Shadow approximates a Gaussian blur applied to a filled rounded rect.
```cpp
//approximation of Blur(radius: 8; offset: 0,1; content: Fill(corner: 12; color: 0,64));
fg: Shadow(blur: 8; corner: 12; offset: 0,1; color: 0,64);
```

Example usage:
```cpp
bg: Shadow(blur: 8; corner: 8; offset: 0,1; color: 0,64);Fill(corner: 8; color: 255),Border(corner: 8; color: 200);
```

<a id="sym-1016746787643331845"></a>

### Split

```cpp
void Split(Layers content, Key32 axis, Float32[] ratio);
```

Experimental
<a id="sym-8974791055023019269"></a>

### Text

```cpp
void Text(Margin& indent, Colour& color, Key32 font, TextProperty& value, Alignment justify, Key32& autofit, Float32& line_space, Float32& line_height, Key32& transform, CString value, Key32& overflow);
```

<a id="sym-9244992215028077829"></a>

### TextEdit

```cpp
void TextEdit(Margin& indent, Colour& color, Key32 font, TextProperty& value, Alignment justify, Key32& autofit, Float32& line_space, Float32& line_height, Key32& transform, Colour& selection_color, Colour& selected_text_color);
```

<a id="sym-8974807998669001989"></a>

### Tile

```cpp
void Tile(Layers content, Margin& indent, Key32 axis, Size& stride);
```

<a id="sym-8974808346561352965"></a>

### Tint

```cpp
void Tint(Layers content, Colour& rgb);
```

<a id="sym-10667000374281377029"></a>

### Translate

```cpp
void Translate(Layers content, Key32 unit, Point& offset);
```

<a id="sym-4954868727309931781"></a>

### Triangle

```cpp
void Triangle(Margin& indent, Colour& color, Alignment& direction, Float32& corner, Float32& width);
```

<a id="sym-1689799709461320965"></a>

### TriangleFill

```cpp
void TriangleFill(Margin& indent, Colour& color, Alignment& direction, Float32& corner);
```

<a id="sym-9424123253560972549"></a>

### ViewPortGrid

```cpp
void ViewPortGrid(Colour& color, Key32 axis, Float32& origin, Float32& unit, Float32 minpx, Float32 indent, bool dotted, Key32 font, Colour& text_color, Point text_offset, Float32[] text_offset, Function<WString(Float)>& label);
```

Experimental
---

