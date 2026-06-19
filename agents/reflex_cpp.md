- [Reflex](#sym-14880872606059205893)
  - [Containers](#sym-1585929918896896715)
  - [Debug](#sym-938601454590196427)
  - [Initialisation](#sym-12367712400810670795)
  - [Intrusive](#sym-11653042658938682059)
  - [Notifications](#sym-16995229157590006475)
  - [Object](#sym-14361920786414140107)
  - [String](#sym-15127199571043908299)
  - [Types](#sym-1023247554823027403)
  - [Async](#sym-925588420942854859)
  - [Data](#sym-8972302129234672331)
    - [Compression](#sym-14156319372346449529)
    - [Encoding](#sym-3168210784701439609)
    - [Format](#sym-12916641001534185081)
    - [Hash](#sym-8972919408061111929)
    - [PropertySet](#sym-13039646562789154425)
    - [Serialization](#sym-697804106381713017)
  - [File](#sym-8972647126777690827)
    - [IO](#sym-25178804392695871)
    - [Path](#sym-8974154335712843839)
  - [GLX](#sym-830891113689873099)
    - [Animation](#sym-11673504531626427658)
    - [Events](#sym-12782211053022971146)
      - [Drag & Drop](#sym-5816385031260828211)
    - [Layout](#sym-13854936142299890954)
    - [Styling](#sym-1171591279885255946)
      - [Canvas](#sym-12340501116277972008)
      - [SVG](#sym-830948585109412904)
    - [Widgets](#sym-2955407527103268106)
  - [SIMD](#sym-8974499447388207819)
  - [System](#sym-15152871578414578379)

<a id="sym-14880872606059205893"></a>

##### Reflex

Reflex is the portable C++ framework for building graphical multimedia applications and audio plugins, featuring:
- Layered modular architecture with strict dependencies and minimal external requirements
- Uniform object model with intrusive references, lightweight views, and data-driven properties
- Unified structured data system including JSON, XML, RIFF, and PropertySheet formats
- Cross-platform abstraction layer for desktop apps, mobile apps, and audio plugins
- Native real-time audio and plugin support including VST and AU
- GPU-driven rendering and UI framework with layout, styling, animation, and events
- Hot-reloadable stylesheets, scripting, and integrated developer tooling
- Designed for high performance with small binaries, low memory usage, and deterministic runtime behaviour


### What it's not

Reflex is not a DSP library or an audio-processing toolkit. It provides low-level abstraction over platform and plug-in APIs, but does not include DSP algorithms, synthesis engines, processing graphs, or other domain-specific audio layers. Those systems are expected to be built on top when needed.

### Framework vs library

The most common way to use Reflex is as a framework. Reflex Creator template projects and the Bootstrap namespace provide the standard starting point for applications, with state/view separation, startup wiring, UI initialisation, and project scaffolding already in place.
Due to its modular nature, Reflex may also be used as a library. Lower-level modules can be embedded into existing host architectures and initialised independently, allowing integration into other frameworks such as JUCE.

#### Organisation

Reflex is organised into a set of small, focused sub-libraries with strict one-directional dependencies. Each directory contains a 'require.h' that aggregates the headers of its dependencies.
The source tree is divided into two major parts, reflex and reflex_ext.

### reflex/

The core, canonical framework layer containing the fundamental primitives and primary APIs of Reflex.
Reflex: Core object model, containers, and reference system. Lightweight primitives forming the foundation for all Reflex modules.
System: Cross-platform OS abstraction (files, memory, threading, timing, SIMD detection).
SIMD: Portable SIMD layer exposing unified vector types over platform intrinsics.
Data: Structured data system (PropertySet), serialization, formats (JSON/XML/RIFF), encoding, compression, hashing.
File: File IO utilities, path handling, virtual file system, and shared resource management.
GLX: UI framework: layout, styling, events, and animation system.
VM: Scripting system: VM, compiler, runtime, module loading, and C++ bindings.

### reflex_ext/

Contains higher-level extension modules, helper systems, widgets, tooling, and application scaffolding built on top of reflex/.
Async: Task system with worker threads, scheduling, and HTTP utilities.
File: Extensions to file system (e.g. monolithic archive format, extra utilities).
GLX: Additional UI widgets and helpers built on GLX.
IDE: Integrated debugging, live editing and hot-reload for stylesheets and scripts.
Bootstrap: Application scaffolding, lifecycle management, VFS setup, and project template foundation.

#### Fundamentals & Key Concepts

Reflex is built around a small set of core concepts which are used consistently throughout the framework.  It is imperative to understand these concepts before diving deeper into the framework:
Object model and lifetime: [Reflex > Object](#sym-14361920786414140107)
Intrusive hierarchies and traversal: [Reflex > Intrusive](#sym-11653042658938682059)
Containers, views, and allocation: [Reflex > Containers](#sym-1585929918896896715)
Strings and text processing: [Reflex > String](#sym-15127199571043908299)
Keys and properties: [Reflex > Data > PropertySet](#sym-13039646562789154425)
Initialisation and globals: [Reflex > Initialisation](#sym-12367712400810670795)

#### Getting Started

A good starting point is the examples/Notes project. It is a small but complete working application demonstrating the separation between UI and application logic, recommended project structure, and typical usage of Reflex modules across GLX, Data, and Bootstrap.
Once familiar with the example, create your own project using Reflex Project Creator (bin/tools), which sets up the required modules, entry point, and Bootstrap scaffolding automatically.
From there, the natural development path is:
- Build your UI using Reflex::GLX - layout, styling, and events
- Manage structured state with Data::PropertySet and the serialization helpers
- Follow the patterns established in the Notes example for state/view separation and lifecycle management

---

<a id="sym-1585929918896896715"></a>

##### [Reflex](#sym-14880872606059205893) > Containers

Reflex provides a compact suite of container types designed around predictable behaviour, explicit allocation control, contiguous storage, and lightweight non-owning views.

#### Design Principles

- Runtime allocator model rather than allocator template parameters
- Explicit allocation policies for deterministic memory behaviour
- Zero-copy views and slicing where possible
- Contiguous storage preferred for iteration efficiency and cache locality
- Minimal and internally consistent APIs


#### Core Containers


### Array

Array<T> is the primary dynamic container in Reflex.
It provides a contiguous dynamically-sized buffer with explicit allocation control and fast iteration.
CString and WString are typedefs of Array<char> and Array<wchar_t>.
- Push / Insert / Remove operations
- Explicit allocation policies such as kAllocateOver, kAllocateExact, and kAllocateNone
- Runtime allocator selection without affecting the container type itself
- Contiguous storage

```cpp
Array<Int32> values;
values.Push(10);
values.Push(20);
```


### Strings

CString and WString are specialised Array types with guaranteed null termination after all modifications.
This allows direct interoperability with traditional C APIs while preserving the Array API model.
```cpp
CString text = "hello";
FILE * file = fopen(path.GetData(), "r");
```

Unlike generic Array<T>, string containers always maintain an additional null terminator internally.

### Views

ArrayView<T> provides a lightweight non-owning slice into contiguous memory.
CString::View and WString::View are typedefs of ArrayView<char> and ArrayView<wchar_t>.
Views are passed by value, never allocate, and are commonly used for substring operations, tokenization, temporary references, and zero-copy APIs.
- Zero-allocation slicing
- Efficient substring and buffer operations
- Lightweight function arguments and return values
- Does not own memory

```cpp
auto [a, b] = Splice(str, 5);
CString copy = a; // promote to owning string if needed
```

The referenced memory must remain valid for the lifetime of the view.

### Sequence

Sequence<TKey, TValue> is a contiguous associative container where keys are not required to be unique.
It is useful when insertion order matters or when multiple entries may share the same identifier.
- Supports repeated keys
- Contiguous storage
- Fast iteration
- Shares the same allocator model as Array


### Map

Map<TKey, TValue> is a simplified unique-key associative container built on top of Sequence.
- Unique keys
- Linear contiguous storage
- Simple map[key] syntax


### Queue

Queue<T> is a fixed-size SPSC (single producer / single consumer) ring buffer.
It is designed for real-time and multi-threaded systems such as audio processing pipelines.
- Lock-free and wait-free operations
- Zero allocations after creation
- Deterministic runtime behaviour


#### Allocation Model


### Runtime Allocators

Reflex containers use allocator objects at runtime rather than allocator template parameters.
This allows containers to move across API and module boundaries without allocator-dependent types.

### Allocation Policies

Write operations such as Push() and Insert() accept explicit allocation policies.
Common policies include:
- kAllocateOver - grow capacity automatically
- kAllocateExact - resize exactly as requested
- kAllocateNone - forbid allocation


#### Iteration

Reflex containers support range-based iteration:
```cpp
for (auto & value : array)
{
}
```

Reverse iteration helpers are also provided via rbegin() and rend().

### Functions

[ToRegion](#sym-2914517720508889803), [ToView](#sym-15265494386943949515)

### Object Types

[Allocation](#sym-6570175900388915915), [Allocator](#sym-13055918718868224715)

### Value Types

[Array](#sym-925399584115751627), [ArrayRegion](#sym-2445480886326753995), [ArrayView](#sym-17111432006044187339), [Map](#sym-830922256497736395), [Queue](#sym-1007300444332194507), [Sequence](#sym-9501096024522062539)
---

<a id="sym-938601454590196427"></a>

##### [Reflex](#sym-14880872606059205893) > Debug


### Value Types

[Output](#sym-14460294615737268939)
---

<a id="sym-12367712400810670795"></a>

##### [Reflex](#sym-14880872606059205893) > Initialisation

Reflex uses a controlled runtime initialisation model. Some systems, including allocators, modules, null-instances, and global state, are not safe to use before Reflex has initialised.

#### Static Initialisation

Avoid global objects which allocate memory or depend on Reflex runtime state.
Allocating types such as Array, CString, WString, Reference, TRef, and many Object-derived types may depend on systems which are not yet initialised during static construction.

#### Recommended Patterns

- Use constexpr CString::View or WString::View for global string constants
- Use The<T>::Acquire() for singleton-style global objects
- Use AcquireProperty<T>(Bootstrap::global, "id") for shared application state
- Use Reflex::Detail::Module when explicit module initialisation order is required


#### Allocating Globals

If a non-trivial global object is unavoidable, ensure the default allocator is instantiated in the same translation unit before the global object.
```cpp
REFLEX_INSTANTIATE_DEFAULT_ALLOCATOR;
CString g_name = "example";
```

In general, prefer explicit initialisation over global construction.
---

<a id="sym-11653042658938682059"></a>

##### [Reflex](#sym-14880872606059205893) > Intrusive

Reflex provides a small set of intrusive container primitives: Item, List, and Node.
These form the structural foundation of many higher-level framework systems including GLX::Object, GLX::Style, property hierarchies, and other tree-based APIs.
Most applications will not implement these classes directly, however you will interact with them implicitly when iterating over children, siblings, or hierarchical structures throughout the framework.

#### Intrusive Containers

An intrusive container stores linkage directly inside the object itself rather than inside external wrapper nodes.
Objects therefore remain allocated exactly where they were created while simultaneously participating in lists or hierarchies.
This model provides:
- No wrapper-node allocations
- Stable object addresses and references
- Lightweight traversal and iteration
- Efficient parent/child hierarchies
- A unified structural model across the framework

Reflex uses intrusive structures extensively for UI trees, style trees, property hierarchies, and related systems.

#### Core Types


### List

List<TYPE, RETAIN, BASE> is a doubly-linked intrusive list.
The list itself does not allocate nodes - linkage exists inside the contained objects.
Common operations include:
- GetFirst()
- GetLast()
- GetNumItems()
- Range-based iteration

```cpp
for (auto & item : list)
{
}
```


### Item

Item represents a single element participating inside a List.
Objects which should appear in intrusive lists inherit from Item.
Common navigation helpers include:
- GetPrev()
- GetNext()


### Node

Node combines both Item and List to form a hierarchical tree node.
A Node may simultaneously:
- Exist as a child inside another Node
- Contain child Nodes of its own

This forms the basis of Reflex hierarchical systems such as UI trees and style trees.
```cpp
for (auto & child : object)
{
	// ...
}
```


#### Ownership and Retention

Depending on template configuration, intrusive lists may optionally retain contained objects automatically.
Many higher-level Reflex systems therefore combine intrusive hierarchy management with the Object reference counting system.
The exact ownership behaviour depends on the RETAIN template parameter and the surrounding framework subsystem.

#### Iteration

Intrusive lists and nodes support standard range-based iteration:
```cpp
for (auto & child : object)
{
}
```

Reverse iteration helpers are also available via rbegin() and rend().

### Value Types

[Item](#sym-8973160663132371659), [List](#sym-8973574272777943755), [Node](#sym-8973908842140367563)
---

<a id="sym-16995229157590006475"></a>

##### [Reflex](#sym-14880872606059205893) > Notifications

Reflex provides two lightweight notification primitives that together cover the majority of notification and change-tracking use cases. Both are intentionally minimal and give explicit control over notification semantics, lifetime, and cost.

#### State & Monitor

Pull-based change tracking.
Extremely lightweight and allocation-free. Clients explicitly poll a State for changes using a State::Monitor. Best suited for high-frequency or frame-based updates where control and predictability are critical, as well as multi-threaded scenarios.

#### Signal

Push-based notification.
Observers register callbacks (including lambdas) and are notified synchronously when the signal is emitted. Listener lifetime is managed automatically via reference-counted handles, making the client passive and convenient. Typically the caller will store the object received CreateListener via a Reference to keep it alive.
Signal is generally more expensive than State and should be avoided for very frequent notifications.

#### Usage

- Use State when changes occur often, when polling is already part of the update loop, when minimal overhead is required, or when notifications may cross thread boundaries.
- Use Signal in single-thread scenarios when events are infrequent, when callbacks improve clarity, or when automatic listener lifetime management is desired.


### Value Types

[Monitor](#sym-3887337300172356192), [Signal](#sym-15069494894420785867), [State](#sym-1017314229302295243)
---

<a id="sym-14361920786414140107"></a>

##### [Reflex](#sym-14880872606059205893) > Object

Reflex makes a primary, explicit distinction between object-types and value-types. Object types derive from Reflex::Object, are shared, reference counted, typically heap-based (although they can also be instantiated on the stack), and non-copyable by default.
Value types (non-Objects such as UInt32 or Array) are lightweight stack-based primitives. To promote a value into an object so it can participate in reference counting, heap allocation, dynamic properties, or generic object APIs, use ObjectOf<TYPE>.
Reflex::Object provides two foundational capabilities:
- Deterministic lifetime management via intrusive reference counting
- A generic interface for attaching and querying typed properties dynamically


#### Object Lifetime

Reflex uses an intrusive reference counting model built directly into Object.
Unlike std::shared_ptr, COM, or Objective-C, there are no external control blocks or hidden allocations - the reference count lives inside the object itself.

### Core Principles

- All reference-counted types derive from Object
- The reference count is stored inside the object itself
- Newly created objects always start with a reference count of zero
- Every Release() must therefore correspond to a previous Retain()
- Ownership semantics are primarily expressed through Reference<T> and TRef<T>


### Reference <TYPE>

Reference<T> is the primary ownership wrapper for Reflex objects. It retains on construction, releases on destruction, and guarantees a valid object reference over its lifetime.
See [Reflex::Reference](#sym-1695362219560368843) for full semantics and usage.

### TRef <TYPE>

TRef<T> is the lightweight non-owning counterpart to Reference<T>. It does not retain or release, but still follows the Reflex convention of representing a valid object reference rather than a nullable pointer.
See [Reflex::TRef](#sym-8974699438245378763) for full semantics and usage.

### Object Creation

Reflex objects should never be created with raw new/delete directly.
Instead, object creation is performed through the Reflex helper APIs:
```cpp
auto a = New<MyClass>(args...);
auto b = Make<MyClass>(args...);
```

- New<T>() returns a TRef<T>
- Make<T>() returns a Reference<T>
- If T is abstract, New<T>() automatically resolves through T::Create()

Make<T>() is generally the most convenient form when immediate ownership is required.

### AutoRelease Helpers

AutoRelease() provides a shorthand for constructing temporary Reference<T> wrappers.
The following examples are equivalent:
```cpp
Reference<HttpConnection> conn = HttpConnection::Create(...);
auto conn = AutoRelease(HttpConnection::Create(...));
auto conn = Make<HttpConnection>(...);
```


### Stack-Allocated Objects

Unlike traditional COM-style systems, Reflex objects may be instantiated directly on the stack.
```cpp
Data::PropertySet node;
```

Stack objects ignore Retain() and Release() calls and are destroyed normally via scope lifetime.
This flexibility is useful and efficient for temporaries and embedded members, however ownership must remain explicit: any retained reference must not outlive the stack object it refers to. For example, storing a Reference or TRef to a stack object beyond its lifetime is invalid:
```cpp
Data::PropertySet node;
auto props = New<Data::PropertySet>();
props->SetProperty("child", node); // invalid after node leaves scope
```


### Null Instances

Reference<T> and TRef<T> default construct using the type's null-instance rather than nullptr.
If a type does not define a null-instance, the wrappers cannot be default-constructed, enforcing validity at compile time.
User-defined object types therefore often require explicit construction:
```cpp
Reference <View> view1 = New<View>();
Reference <View> view2 = kNewObject;	//shorthand helper
```

TRef<T> also supports construction using kNoValue for deferred assignment, however this should only be used sparingly.

### Manual Retain/Release

Object exposes Retain() and Release(), however direct usage is uncommon.
If manual lifetime management is required:
- New objects begin with refcount = 0
- Every Release() must correspond to a previous Retain()
- Double-release will assert
- Retaining in constructors and releasing in destructors is the standard pattern


### Circular References

Reflex is not a garbage collected system. Circular references will leak unless explicitly broken:
```cpp
auto a = New<Data::PropertySet>();
auto b = New<Data::PropertySet>();
a->SetProperty("foo", b);
b->SetProperty("foo", a);
```

Some advanced systems inside Reflex (such as Reflex::VM) provide explicit cycle-breaking mechanisms, however application code should generally avoid circular ownership through design.

### Best Practices

- Prefer Reference<T> for ownership
- Use TRef<T> for temporary non-owning references
- Avoid manual Retain()/Release() where possible
- Avoid circular ownership relationships
- Prefer clear ownership hierarchies


### ObjectOf<T>

ObjectOf<T> wraps a value type inside an Object, allowing values to participate in reference counting, dynamic properties, and generic object-based APIs.

#### Generic Properties

Reflex::Object defines a virtual interface for attaching, querying, and removing typed properties dynamically via Object::SetProperty, Object::QueryProperty, and Object::UnsetProperty.
This allows sub-objects and arbitrary typed values to be attached to objects at runtime, reducing the need for deep subclassing and enabling highly data-driven systems.
Properties are identified by both id AND type, rather than by id alone. This complements C++'s strong typing model by allowing multiple typed views of the same conceptual property.
Important: Object Does Not Implement the Interface
Reflex::Object itself does not provide a concrete property implementation. Calling SetProperty() on a derived type which does not implement the property callbacks will silently discard the property.
Typically, dynamic properties are implemented through Data::PropertySet, which fully implements the property system for arbitrary types.
Many primary framework classes (such as GLX::Object) derive from Data::PropertySet and therefore support dynamic properties automatically.
For more information on dynamic properties, see [Data::PropertySet](#sym-13039646562789154425).

### Functions

[AcquireProperty](#sym-18172314581995846347), [AutoRelease](#sym-8453966097060952779), [GetAbstractProperty](#sym-13647811632263559883), [GetProperty](#sym-8052652234187700939), [Make](#sym-8973690004966701771), [New](#sym-830927530717575883), [SetAbstractProperty](#sym-550441268363360971), [UnsetAbstractProperty](#sym-17365949766550387403)

### Object Types

[Object](#sym-14361920786414140107)

### Value Types

[ConstReference](#sym-16417332563359791819), [ConstTRef](#sym-532369501975706315), [ObjectOf](#sym-15739513838455978699), [Reference](#sym-1695362219560368843), [TRef](#sym-8974699438245378763)
---

<a id="sym-15127199571043908299"></a>

##### [Reflex](#sym-14880872606059205893) > String

In Reflex, strings are arrays.
CString and WString are typedefs of Array<char> and Array<wchar_t> respectively, while CString::View and WString::View are typedefs of ArrayView<char> and ArrayView<wchar_t>.
Because of this, most string utilities operate generically on Array and ArrayView rather than on dedicated string-only types.
Functions such as Split, Splice, Search, Replace, Trim, and tokenization helpers therefore work uniformly across strings, byte arrays, and other contiguous sequences.

#### String Storage

CString and WString are specialised Array types which guarantee null termination after all modifications.
This allows safe interoperability with traditional APIs expecting const char* or const wchar_t* buffers.
```cpp
CString text = "hello";
SomeAPI(text.GetData());
```

Generic Array<T> containers do not provide this guarantee.

#### Views

Many string operations return lightweight non-owning views rather than allocating new strings.
This allows substring and tokenization operations to occur without copying or heap allocation.
```cpp
CString str = "HelloWorld";
auto [a, b] = Splice(str, 5);
```

In this example, both a and b reference memory owned by str.
If str is modified or destroyed, the views immediately become invalid.
This behaviour is intentional and allows high-performance zero-copy APIs throughout the framework.

#### Ownership

When ownership of a substring or slice is required, the view must be explicitly copied into an owning string:
```cpp
CString str = "HelloWorld";
auto [a, b] = Splice(str, 5);

CString owned = b;
```

Explicit copying keeps ownership semantics visible and avoids hidden allocations in performance-sensitive code paths.

#### Generic Sequence Utilities

Because strings are simply specialised arrays, most helper functions are implemented generically for contiguous sequences rather than specifically for text.
This keeps the API surface small and consistent while avoiding duplicate implementations for strings, byte buffers, and related container types.

#### Iteration

Strings and views support standard range-based iteration:
```cpp
for (auto c : text)
{
}
```

Reverse iteration helpers are also available via rbegin() and rend().

### Typedefs

[CString](#sym-17531698610406914763), [CString::View](#sym-2022350079943473867), [WString](#sym-17787487981880441547), [WString::View](#sym-5727895666471172811)

### Functions

[Join](#sym-8973292196505811659), [Left](#sym-8973553721359432395), [Lowercase](#sym-12955598882812951243), [Merge](#sym-984515724431293131), [Mid](#sym-830923338829494987), [RawStringCopy](#sym-5441740889099827915), [RawStringLength](#sym-6112432979064355531), [Remove](#sym-14881969406732368587), [Replace](#sym-11504363754578213579), [ReverseSearch](#sym-7135086472021113547), [ReverseSplice](#sym-7192770605815659211), [Right](#sym-1010549333688548043), [Search](#sym-15048213812505443019), [Splice](#sym-15105897946299988683), [Split](#sym-1016746791108049611), [ToCString](#sym-10018937018765245131), [ToFloat32](#sym-12386713323243272907), [ToFloat64](#sym-12386713757034969803), [ToInt32](#sym-5633841749614099147), [ToInt64](#sym-5633842183405796043), [ToUInt32](#sym-3277278771522271947), [ToUInt64](#sym-3277279205313968843), [ToWString](#sym-10274726390238771915), [Uppercase](#sym-11355172447734430411)
---

<a id="sym-1023247554823027403"></a>

##### [Reflex](#sym-14880872606059205893) > Types


### Typedefs

[Key32](#sym-974353891938499275)

### Value Types

[Address](#sym-9214008371664091851), [Float32](#sym-1452730841175390923), [Float64](#sym-1452731274967087819), [Function](#sym-5470074934380427979), [Idx](#sym-830904007181695691), [Int16](#sym-965532389889768139), [Int32](#sym-965532656177740491), [Int64](#sym-965533089969437387), [Int8](#sym-8973134498191604427), [Key](#sym-830913507649354443), [NullType](#sym-12240650208584891083), [Pair](#sym-8974152822052584139), [Point](#sym-1001298644147862219), [Rect](#sym-8974479385595968203), [Size](#sym-8974655638168894155), [Tuple](#sym-1022631093872065227), [UInt16](#sym-15243775085220428491), [UInt32](#sym-15243775351508400843), [UInt64](#sym-15243775785300097739), [UInt8](#sym-1020924849394252491), [WChar](#sym-1030155236624530123)
---

<a id="sym-925588420942854859"></a>

##### [Reflex](#sym-14880872606059205893) > Async

Async is a small util library built on top of the low-level System::Task and System::Thread primitives, providing high-level and convenient access to asynchronous operations.  The API is designed to simplify correct async usage while keeping control over execution, lifetime, and thread safety explicit.
It covers the most common async use cases, including background execution with progress reporting and cancellation, HTTP requests, safe transfer of payloads across thread boundaries, and lifetime management of running tasks.

#### Lifetime

Async tasks are reference-counted. Clients typically hold a Reference <Async::Task>.  Destroying this reference automatically requests cancellation of the task (assuming one-owner).
For custom workers, task cancellation is cooperative. Worker implementations must explicitly observe and respect the task’s run flag via calling Cancelled() and if true, return early. In this case typically you should also ctx.SetResult(false) to indicate failure.

#### Observation

Progress and completion are commonly monitored via polling using clocks created with CreateClock() or CreatePeriodic().
However the recommended solution is to use the AttachAwait helper function which handles this pattern safelt, including use of weak references for handling edge cases where the completion callback destroys the observer.
In UI code, do not use Async::CreateClock or Async::CreatePeriodic. Use the GLX equivalents (GLX::CreateAnimationClock, GLX::CreatePeriodicClock, or GLX::Object::OnClock), which guarantee a valid UI context for each callback.

#### Examples

```cpp
//start a background task
auto task_ref = Make<Async::Worker>([](Async::Worker::Context & ctx)
{
	auto result = New<Data::UInt32Property>(); //use whatever payload type is suitable

	UInt n = 0;
	while (!ctx.Cancelled() && n < 100)
	{
		System::SuspendThread(10);             //emulate work

		ctx.SetProgress(Float(n) / 99.0f);     //publish progress for UI

		n++;
	}

	result->value = n;

	ctx.SetResult(n == 100, result);
});

// monitor progress
m_clock_ref = Async::CreatePeriodicClock(0.1f, [task_ref]()	//!by capturing task_ref, the Task is kept alive as long as needed
{
	switch (task_ref->GetStatus())
	{
	case Async::Task::kStatusCompleted:
		if (auto result = Cast<Data::UInt32Property>(task_ref->GetResult()))	//use DynamicCast if your worker returns different types
		{
			//do something with result
		}
		break;

	default:
		break;
	}
});
```


### Functions

[AttachAwait](#sym-11982832383702358589), [CancelAwait](#sym-2674671305808173629), [CreateClock](#sym-16961170023498406461), [CreatePeriodicClock](#sym-16352619813884942909)

### Enums

[Status](#sym-15124628141771903332)

### Object Types

[Task](#sym-8974771601801239101), [Worker](#sym-15774081169288616509)

### Value Types

[Context](#sym-3761071570620451947)
---

<a id="sym-8972302129234672331"></a>

##### [Reflex](#sym-14880872606059205893) > Data

The Reflex::Data namespace provides the core infrastructure for representing, transforming, and persisting structured data in a platform-independent way.
It sits beneath higher-level systems and is used throughout Reflex wherever data needs to be serialized, stored, transmitted, hashed, or converted between representations.
Data covers three main concerns: structured data representation, format-level serialization, and low-level data transformation utilities.
- Structured data: PropertySet, generic properties, and reflection-friendly containers used across the engine.
- Serialization & formats: Pluggable Format implementations for JSON, XML, RIFF, Reflex PropertySheet, and binary PropertySet.
- Data transformation: Compression, hashing, and string encoding utilities (e.g. EncodeUTF8).

You should expect to use the Data namespace extensively in application logic. When used correctly, it removes the need to roll your own data containers, serialization layers, encoding utilities, or hashing functions, as these concerns are already handled in a consistent and well-integrated way across Reflex.
---

<a id="sym-14156319372346449529"></a>

##### [Reflex](#sym-14880872606059205893) > [Data](#sym-8972302129234672331) > Compression


### Globals

[kLZ4](#sym-8978219608157773433)

### Functions

[Compress](#sym-13248356113376665209), [Decompress](#sym-141847145697304185)

### Object Types

[CompressionAlgorithm](#sym-7545490347245231737), [DecompressionAlgorithm](#sym-487480056112799353)
---

<a id="sym-3168210784701439609"></a>

##### [Reflex](#sym-14880872606059205893) > [Data](#sym-8972302129234672331) > Encoding


### Functions

[BytesToHex](#sym-14859215565183049337), [DecodeUCS2](#sym-17015155771860639353), [DecodeUTF8](#sym-17015233467819023993), [DecodeUrlSegment](#sym-4808060509841579641), [EncodeUCS2](#sym-616012591737659001), [EncodeUTF8](#sym-616090287696043641), [EncodeUrlSegment](#sym-896686063126371961), [HexToBytes](#sym-6238121638285598329), [IsHttps](#sym-627316825401646713), [MakeUrl](#sym-1518101572397096569), [SplitUrl](#sym-14276455779483573881), [SplitUrlResource](#sym-3947668733844053625)

### Value Types

[Url](#sym-830962064086326905)
---

<a id="sym-12916641001534185081"></a>

##### [Reflex](#sym-14880872606059205893) > [Data](#sym-8972302129234672331) > Format

Data::Format defines the interface for encoding and decoding structured data.  A given Format implements how to serialize a PropertySet into bytes and how to reconstruct it back into a PropertySet.
Reflex provides several global, constant Format instances covering the common persistence use cases.

#### Common Formats


### kPropertySetFormat

- Recommended binary format
- Fast, compact, and supports the widest range of Reflex property objects: PropertySet, BoolProperty, UInt32Property, UInt64Property, Int32Property, Int64Property, Float32Property, Float64Property, CStringProperty, WStringProperty, Key32Property, ArchiveObject, and KeyMap.


### kPropertySheetFormat

- Human-readable text format
- Similar to JSON but with additional type info support.
- Supports PropertySet, PropertySetArray, BoolProperty, UInt32Property, UInt64Property, Int32Property, Int64Property, Float32Property, Float64Property, CStringProperty, WStringProperty, Key32Property, ArchiveObject, and KeyMap.


### kJsonFormat

- Standard JSON
- Ideal for interoperability with web APIs and external tooling.
- Supports PropertySet, PropertySetArray, BoolProperty, UInt32Property, UInt64Property, Int32Property, Int64Property, Float32Property, Float64Property, CStringProperty, WStringProperty, and KeyMap. Unsupported values are emitted as null.


### kReflexXmlFormat

- Supports a sub-set of XML
- Only attributes and nested elements (ignores text content between tags).
- Supports PropertySetArray, CStringProperty, BoolProperty, Int32Property, Float32Property, and KeyMap. Nested node payloads are represented as PropertySet children.


#### Typical Usage


### Encoding

```cpp
Data::PropertySet data;

Data::SetInt32(data, "version", 1);

auto child = Data::AcquirePropertySet(data, "child");
Data::SetFloat32Array(child, "values", {1.0f, 2.0f});

auto blob = Data::EncodePropertySet(Data::kPropertySetFormat, data);

File::Save(path, blob);
```


### Decoding

```cpp
auto bytes = File::Open(path);

Data::PropertySet root = Data::DecodePropertySet(Data::kPropertySetFormat, bytes);
if (root)
{
	auto v = Data::GetInt32(root, "version");
	auto sub = Data::GetPropertySet(root, "child");
	auto arr = Data::GetFloat32Array(sub, "values");
}
```


#### Choosing a Format

- Use kPropertySetFormat for application data, presets, configs, or anything Reflex-internal.
- Use kPropertySheetFormat when you need editable text files or want structured types without JSON’s restrictions.
- Use kJsonFormat for web communication and external file formats.


### Globals

[kBinaryFormat](#sym-1018804717929684601), [kJsonFormat](#sym-6310041514197180025), [kPropertySetFormat](#sym-16767242442695040633), [kPropertySheetFormat](#sym-14488901298783575673), [kReflexMarkupFormat](#sym-13193545862458957433), [kReflexXmlFormat](#sym-1588972630179896953)

### Functions

[CopyPropertySet](#sym-12382521355389689465), [DecodePropertySet](#sym-4196140279909113465), [EncodePropertySet](#sym-4247992054274121337), [ResetPropertySet](#sym-17882451587527401081)

### Object Types

[Format](#sym-12916641001534185081), [SerializableFormat](#sym-5671468602716380793)

### Value Types

[DeserializeError](#sym-599668559877486605)
---

<a id="sym-8972919408061111929"></a>

##### [Reflex](#sym-14880872606059205893) > [Data](#sym-8972302129234672331) > Hash

Reflex provides a small set of non-cryptographic hash functions for use in lookup keys, content identifiers, and integrity checks.
These are designed for speed and practicality, not for cryptographic security.
All functions operate on Archive::View and return an Archive (for SHA) or UInt32/UInt64.

### Functions

[CRC32](#sym-930420767788686969), [FNV1a32](#sym-14722083040447749753), [FNV1a64](#sym-14722083474239446649), [SHA1](#sym-8974492985294118521), [SHA256](#sym-14895257324413312633)
---

<a id="sym-13039646562789154425"></a>

##### [Reflex](#sym-14880872606059205893) > [Data](#sym-8972302129234672331) > PropertySet

Data::PropertySet is Reflex’s generic container for structured, tree-based data.
PropertySet allows arbitrary data to be attached to objects at runtime, without modifying class definitions or introducing ad-hoc subclasses.
Instead of encoding every variation in a type hierarchy, behavior and state can be composed dynamically by attaching properties as needed.
```cpp
object.SetProperty("hover_time", 0.0f);
object.SetProperty("user_data", some_ref);
```

This bridges the performance and safety of C++ with the flexibility typically associated with dynamic languages.  It enables rapid iteration, late-bound features, and data-driven behavior while remaining fully type-aware and debuggable.

#### Generic property system

PropertySet implements the root Reflex::Object property interface (OnSetProperty / OnUnsetProperty / OnQueryProperty).
This provides a uniform, generic property mechanism across the entire framework.
Properties are identified by id AND type, rather than id alone.
This complements C++'s strong typing model by allowing multiple, type-safe views of the same conceptual property without collapsing everything into a single loosely-typed value.
```cpp
ps.SetProperty("my_id", 1);
ps.SetProperty("my_id", "string");
```

The example above creates two distinct properties:
- An Int32 property with id "my_id".
- A CString property with id "my_id".

Under the hood, this (id + type) pair is represented by [Reflex::Address](#sym-9214008371664091851).

#### Structured and hierarchical data

PropertySet supports hierarchical data by allowing nested PropertySet instances.
This makes it suitable for representing structured trees of runtime state, configuration, or metadata when needed.
Unlike rigid schemas, PropertySet allows structure to evolve organically as systems interact.

#### Persistence and serialization

PropertySet is serializable, via the Format system.
This makes it suitable for storing presets, state snapshots, configuration files, and interchange data.
For persistent data, values should be written using the Data::SetXXX family of functions, which define the set of interoperable, format-safe property types. Using the Reflex::Data:: suite of property accessors also avoids template bloat occuring from use of the Reflex:: templated ones.
```cpp
Data::SetFloat32(ps, "gain", 0.75f);
Data::SetCString(ps, "name", "Preset A");
```

PropertySet can be serialized using any Data::Format implementation:
```cpp
auto blob = Data::EncodePropertySet(Data::kPropertySetFormat, ps);
//or
auto json = Data::DecodePropertySet(Data::kJsonFormat, ps);
```

Note that when writing text-based formats (JSON, Reflex PropertySheets) you need to register the key-strings, via Data::AcquireKeyMap and Data::RegisterKey.

#### Summary

- PropertySet enables dynamic, runtime data attachment without subclassing.
- It is a core mechanism for flexible, data-driven behavior in Reflex.
- Properties are identified by (id, type), allowing rich runtime composition.
- Serialization is supported via pluggable formats.

Use Iterate<TYPE>() to iterate over all properties of TYPE
Each item will have a [key,value] where key is Reflex::Address (comprising of the property id and type_id) and value a Reference <TYPE>
```cpp
for (auto & [adr, ref] : test.Iterate<Data::CStringProperty>())
{
	output.Log(id.value, ref->value);
}
```


### Typedefs

[Float32Property](#sym-6318097438325399161), [Float64Property](#sym-5532383564373483129), [Int32Property](#sym-4261681755723980409), [Int64Property](#sym-3475967881772064377), [KeyMap](#sym-13707062997161139833), [UInt32Property](#sym-9719418972175130233), [UInt64Property](#sym-8933705098223214201)

### Functions

[AcquireKeyMap](#sym-5361974431892692601), [AcquirePropertySet](#sym-6835714230980763257), [AcquirePropertySetArray](#sym-9716146481973485177), [AddPropertySet](#sym-103905494798753401), [Assimilate](#sym-5360487183502335609), [GetBool](#sym-5843614929209319033), [GetFloat32](#sym-7490243057635352185), [GetFloat64](#sym-7490243491427049081), [GetInt32](#sym-8407367511912275577), [GetInt64](#sym-8407367945703972473), [GetKey](#sym-13033941574909746809), [GetKey32](#sym-8416188747673034361), [GetKeyMap](#sym-1033196285755186809), [GetPropertySet](#sym-14089334233469345401), [GetPropertySetArray](#sym-5537952500573855353), [GetUInt32](#sym-2569908637639171705), [GetUInt64](#sym-2569909071430868601), [GetUInt8](#sym-8462759705128787577), [Merge](#sym-984515721968017017), [RegisterKey](#sym-12302724445032734329), [SetBinary](#sym-8029977604720221817), [SetBool](#sym-17065134996319166073), [SetCString](#sym-8616923115222523513), [SetFloat32](#sym-10984699419700551289), [SetFloat64](#sym-10984699853492248185), [SetInt32](#sym-9782648252346195577), [SetInt64](#sym-9782648686137892473), [SetKey32](#sym-9791469488106954361), [SetPropertySet](#sym-9982811737137667705), [SetUInt32](#sym-11060684924539428473), [SetUInt64](#sym-11060685358331125369), [SetUInt8](#sym-9838040445562707577), [SetWString](#sym-8872712486696050297), [UnsetBinary](#sym-18203019975041539705), [UnsetBool](#sym-12399268768611622521), [UnsetCString](#sym-12285928009054084729), [UnsetFloat32](#sym-14653704313532112505), [UnsetFloat64](#sym-14653704747323809401), [UnsetInt32](#sym-3383015327673671289), [UnsetInt64](#sym-3383015761465368185), [UnsetKey32](#sym-3391836563434430073), [UnsetPropertySet](#sym-15730379041849532025), [UnsetPropertySetArray](#sym-9256090634860682873), [UnsetUInt32](#sym-2786983221151194745), [UnsetUInt64](#sym-2786983654942891641), [UnsetUInt8](#sym-3438407520890183289), [UnsetWString](#sym-12541717380527611513)

### Object Types

[ArchiveObject](#sym-7146817876374118009), [ObjectArray](#sym-16772990762499497593), [PropertySet](#sym-13039646562789154425)

### Value Types

[PropertyIterator](#sym-8760467420767618014)
---

<a id="sym-697804106381713017"></a>

##### [Reflex](#sym-14880872606059205893) > [Data](#sym-8972302129234672331) > Serialization

Reflex serialization provides a lightweight, explicit way to encode and decode strongly-typed data to and from a binary stream.
It is designed for deterministic layouts, low overhead, and full control over what is written.
Unlike PropertySet serialization (which targets structured, schema-light data), this API is intended for **compact, ordered, binary serialization** of known data layouts.

#### Archive

Data::Archive is a byte container used as the backing store for serialization.
It is a simple typedef over Array<UInt8>, and can be treated as a writable or readable byte stream.
```cpp
Data::Archive outstream;
```

To read from an archive, use Data::Archive::View, which maintains a read cursor without copying data:
```cpp
Data::Archive::View instream = outstream;
```

Archive::View performs deserialization directly from the underlying byte buffer. For simple value types, data is read by copying from the current stream position and advancing an internal read cursor. This avoids additional heap allocations and minimizes intermediate copying, making the operation suitable for performance-sensitive code paths.

#### Writing data (Serialize)

Use Data::Serialize to append strongly-typed values to an archive in sequence.
```cpp
Data::Serialize(stream, 1, 2.0f);
Data::SerializeUTF8(stream, L"wide");
```

Values are written in the order provided. The resulting stream is compact and contains no metadata beyond what is required to decode variable-length data (such as strings).

#### Reading data (Deserialize)

Use Data::Deserialize to read values back in the same order they were written.
```cpp
auto [i, f] = Data::Deserialize<Int32, Float32>(stream);
auto wstring = Data::DeserializeUTF8(stream);
```

Deserialization advances the read cursor stored in the Archive::View.
The template parameters define the expected types and enforce strong typing at the call site.

### Validation

Within Reflex, deserialization fundamentally expects pre-validated input. Individual Data::Deserialize calls do not perform size or integrity checks - this is by design, not omission. A size check at the individual deserializer level is largely meaningless in isolation: confirming that 4 bytes remain before reading a UInt32 says nothing about whether that value is semantically correct.
The general approach therefore is validation at the stream boundary, before deserialization begins. Once a valid, correctly bounded Archive::View is established at the container level, deserializers downstream can and should proceed without checks.
The main exception is the higher-level SerializableFormat::Deserialize path, which does perform stream validation in order to support the Format::Decode-style APIs as those are intended to unpack external input data.  See Data::Format and Data::SerializableFormat for more info.

#### Ordering and determinism

Serialization is order-dependent.
The reader must deserialize values in the exact order and type they were written.
This explicitness is intentional:
- No reflection or schema lookup.
- No runtime type ambiguity.
- No hidden allocations.

This makes the system well suited for file formats, IPC, network packets, caches, and other performance-sensitive data paths.

#### Strings and encoding

String serialization is explicit.
Use SerializeUTF8 / DeserializeUTF8 to encode wide strings into UTF-8 byte sequences.
Encoding is handled deterministically and is independent of platform wchar size.

#### Relationship to PropertySet

Use direct serialization when the data layout is known and fixed.
Use PropertySet serialization (EncodePropertySet) when flexibility, flexible version evolution, or dynamic composition is required.
Both systems share the same Data namespace but target different problem spaces.

#### Relationship to Pack / Unpack

Data::Pack and Data::Unpack provide a lower-level mechanism for converting a single value into a binary representation and restoring it again.
These functions operate on types that have a well-defined, “toll-free” binary representation - meaning the value can be viewed as a contiguous block of bytes without transformation. This typically includes:
- Integral and floating-point types.
- POD and trivially copyable types.
- Arrays of such types.

```cpp
UInt32 t;
Data::Archive::View view = Data::Pack(t);

auto restored = Data::Unpack<UInt32>(view);
```

For types with a compile-time known binary size (for example, UInt32 is always 4 bytes), Pack / Unpack and Serialize / Deserialize produce the same binary representation. Internally, Serialize uses Pack / Unpack for these cases.
The distinction becomes important for variable-sized data.
Serialize is designed for writing *sequences* of values to a stream. As such, it includes any additional information required to reconstruct the data when reading, such as encoding the length of strings or arrays before their contents.
In contrast, Pack produces only the binary representation of the value itself. For example, when packing a string, the length is not prepended, as the size of the binary view implicitly defines the data extent.

#### Summary

- Data::Archive is a simple byte stream for serialization.
- Serialize / Deserialize write and read strongly-typed values in order.
- The API is explicit, deterministic, and allocation-aware.
- String encoding is handled explicitly via UTF-8 helpers.
- PropertySet serialization serves higher-level, structured data needs.


### Typedefs

[Archive](#sym-11560583731558276729), [Archive::View](#sym-3410220346311237241)

### Functions

[Deserialize](#sym-13657719981913202297), [DeserializePropertySet](#sym-4064969010866871929), [DeserializeUCS2](#sym-16231191942463020665), [DeserializeUTF8](#sym-16231269638421405305), [Pack](#sym-8974151939121012345), [ReadLine](#sym-8143884342183124601), [Serialize](#sym-3450658146302877305), [SerializePropertySet](#sym-11122979301999304313), [SerializeUCS2](#sym-713211318008602233), [SerializeUTF8](#sym-713289013966986873), [Unpack](#sym-15432461427091234425), [WriteLine](#sym-7367949705037274745)
---

<a id="sym-8972647126777690827"></a>

##### [Reflex](#sym-14880872606059205893) > File

The Reflex::File namespace provides a unified, cross-platform interface for working with files, paths, and shared file-backed resources.
It is built on top of the System layer and is intended to be the primary API for file and resource access in application code.
File covers three closely related concerns: path manipulation, file I/O, and virtualised/shared resource access.
- Path utilities: helpers for common path operations such as SplitFilename, CheckExtension, ResolveExistingPath, and related functions.
- File I/O: high-level helpers for opening, saving, and reading files (e.g. File::Open, File::Save, File::ReadLine), avoiding direct interaction with low-level file handles.
- Virtual filesystems and resources: abstractions for unifying real, bundled, and embedded data sources.

---

<a id="sym-25178804392695871"></a>

##### [Reflex](#sym-14880872606059205893) > [File](#sym-8972647126777690827) > IO


### Functions

[Copy](#sym-8972212795746687039), [CreateMemoryReader](#sym-15820803427873724479), [CreateMemoryWriter](#sym-16728754609395472447), [GetRemainder](#sym-10722639413154814015), [Open](#sym-8974068045524899903), [Peek](#sym-8974170931466475583), [ReadBytes](#sym-10445329438029781055), [ReadLine](#sym-8143884342183204927), [ReadValue](#sym-10543459708347488319), [Save](#sym-8974617651014932543), [WriteBytes](#sym-3286230485926287423), [WriteLine](#sym-7367949705037355071), [WriteValue](#sym-3384360756243994687)

### Object Types

[PersistentPropertySet](#sym-1931492178932144191), [ResourcePool](#sym-2784316003510786111), [VirtualFileSystem](#sym-3007734335323379775)
---

<a id="sym-8974154335712843839"></a>

##### [Reflex](#sym-14880872606059205893) > [File](#sym-8972647126777690827) > Path


### Globals

[kPathDelimiter](#sym-15702586372901705791)

### Functions

[CheckExtension](#sym-6450198791125604415), [CorrectExtension](#sym-2858318139697010751), [CorrectStrokes](#sym-16588900385359534143), [CorrectTrailingStroke](#sym-5910597418091681855), [Delete](#sym-12528574737652258879), [DeleteDirectoryContent](#sym-4095212319434479679), [DeletePath](#sym-6772400799515547711), [Exists](#sym-12793038813487306815), [GetSystemPath](#sym-9443082047660890175), [GetVolumes](#sym-4497562553621289023), [IsDirectory](#sym-15415839624482920511), [MakeDirectory](#sym-4686556267289909311), [MakePath](#sym-13203014036258691135), [MakeRelativePath](#sym-4205262043121588287), [RemoveDuplicateStrokes](#sym-14766849600538155071), [RemoveTrailingStroke](#sym-12916606465702238271), [Rename](#sym-14882056995832207423), [ResolveExistingFolder](#sym-8927668057832298559), [ResolveRelativePath](#sym-7752503441585477695), [SplitExtension](#sym-2572262434336282687), [SplitFilename](#sym-8578058845219330111)
---

<a id="sym-830891113689873099"></a>

##### [Reflex](#sym-14880872606059205893) > GLX

GLX is Reflex's UI framework. It combines a retained object tree, a declarative layout system, stylesheet-based rendering, an event model, and time-based animation helpers into one consistent API surface.
Most GLX work falls into a small set of concepts which are used repeatedly across the framework:
- Every visible element is a GLX::Object in a parent -> child hierarchy
- Parents control layout flow, while children declare how they participate in that layout
- Visual appearance is defined in stylesheets and render layers rather than hard-coded drawing logic
- Input and application behavior are driven through bubbling events and delegate bindings
- Animated behavior is built from state transitions, explicit animations, and clocks


#### Key Guides

See the following guides before diving into specific widgets or helpers:
Object tree and layout: [Layout](#sym-13854936142299890954)
Stylesheets, layers, and states: [Styling](#sym-1171591279885255946)
Input dispatch and custom behavior: [Events](#sym-12782211053022971146)
Time-based transitions, clocks, and procedural updates: [Animation](#sym-11673504531626427658)

#### Minimal Example

```cpp
auto panel = New<GLX::Object>();
GLX::SetFlow(panel, GLX::kFlowY);

auto title = New<GLX::Label>(L"Overview");
auto body = New<GLX::Object>();
auto button = New<GLX::Button>(L"Run");

GLX::AddInline(panel, title);
GLX::AddInlineFlex(panel, body);
GLX::AddFloat(body, button, GLX::kAlignmentTopRight);
GLX::BindClick(button, [](){});
```

---

<a id="sym-11673504531626427658"></a>

##### [Reflex](#sym-14880872606059205893) > [GLX](#sym-830891113689873099) > Animation

GLX animation covers two closely related systems:
- Animation objects that interpolate values or states over time
- Clocks that execute UI callbacks on the GLX update thread

In many cases the simplest animation is declarative. A stylesheet can define @State variants and a transition time, and code only needs to push or clear the relevant state.

#### State-Based Transitions

For hover, selected, inactive, and similar UI feedback, prefer stylesheet-driven transitions first.
```cpp
Button:
{
	transition: 0.25;

	@State hover:
	{
		bg: Fill(colour: 228);
	};
}
```

This keeps simple UI motion in the style layer rather than in imperative code.

#### Running Explicit Animations

When code needs to decide target values dynamically, create an animation object and run it against a property or state on a target object.
```cpp
auto fade = GLX::CreateColourPropertyAnimation("colour", GLX::kWhite, GLX::kBlack);
GLX::Run(object, "colour", 0.25f, fade);
```

Common helpers in this module include CreateStateAnimation, CreateColourPropertyAnimation, CreateMarginPropertyAnimation, and CreateCallbackAnimation.

#### Clocks

Use clocks when you need procedural updates over time rather than interpolation between two values.
- CreateAnimationClock / AttachAnimationClock for frame-based UI callbacks
- CreatePeriodicClock / AttachPeriodicClock for lower-frequency periodic work
- DetachClock to stop an attached clock

This is commonly used for polling async task status, updating drag visuals, driving custom canvas effects, or other time-based UI behavior that is not well described as a simple property tween.

#### Choosing the Right Approach

- Use stylesheet transition + @State for simple visual feedback
- Use explicit animation objects when code determines the end value or state
- Use clocks for continuous procedural behavior or periodic observation


### Functions

[AttachAnimationClock](#sym-15187249853810967818), [AttachPeriodicClock](#sym-15145119518877123850), [CreateAnimationClock](#sym-18141271455978063114), [CreateCallbackAnimation](#sym-16203876171916608778), [CreateColourPropertyAnimation](#sym-14829502906532726026), [CreateFloatPropertyAnimation](#sym-461854346607428874), [CreateInterpolatedAnimation](#sym-4822352329464448266), [CreateLogarithmicAnimation](#sym-13538936539131774218), [CreateMarginPropertyAnimation](#sym-15922530894628357386), [CreateMaxBoundsAnimation](#sym-10471632103631062282), [CreateOpacityAnimation](#sym-4438300812032181514), [CreatePeriodicClock](#sym-16352619814319129866), [CreatePointPropertyAnimation](#sym-11994997758743381258), [CreatePositionAnimation](#sym-3372461435799045386), [CreateSizePropertyAnimation](#sym-1956205431755998474), [CreateStateAnimation](#sym-14630098535735985418), [CreateWaitAnimation](#sym-3214050947294659850), [DetachClock](#sym-18005836590080951562), [Enter](#sym-945166050058667274), [Exit](#sym-8972562576001041674), [Run](#sym-830948468632683786), [Stop](#sym-8974705575703184650)

### Enums

[Easing](#sym-12677384309400002985)

### Object Types

[Animation](#sym-11673504531626427658), [ContainerAnimation](#sym-13415755210635183370), [InterpolatedAnimation](#sym-17361695219141149962), [Multi](#sym-986959092620821770), [PlayList](#sym-13030293024079447306)
---

<a id="sym-12782211053022971146"></a>

##### [Reflex](#sym-14880872606059205893) > [GLX](#sym-830891113689873099) > Events

The GLX Event System provides a unified way to send and receive input and custom messages across all UI objects.
Events are lightweight, dynamically extensible objects that bubble up through the view hierarchy until handled or consumed.

#### Fundamentals

GLX::Event derives from Data::PropertySet, allowing arbitrary name-value pairs to be attached dynamically.
It also has a Key32 id member for fast event type comparisons.
GLX::Object::Emit delivers an event to the target object, then bubbles it up through its parent hierarchy until a handler traps or consumes it.
Use GLX::Object::ProcessEvent when you want to dispatch an event directly to a single object without bubbling.
It is the callers responsiblity to ensure the event is retained before calling Emit or ProcessEvent.

#### Emitting Events

The low-level approach is to create and populate an Event instance manually and call Emit() on the target object.
```cpp
auto e = Make<Event>("MyCustomEvent");
Data::SetBool(e, "selected", true);
my_view->Emit(e);
```

A more concise helper exists:
```cpp
GLX::Emit(*this, "MyCustomEvent", "selected", true);
```

This ensures compile-time checking that arguments are provided in name-value pairs and that names decay to Key32.

#### Receiving Events

The lowest-level way to respond to events is by overriding bool GLX::Object::OnEvent(GLX::Object & src, GLX::Event & e).
```cpp
bool MyView::OnEvent(GLX::Object & src, GLX::Event & e)
{
	switch (e.id.value)
	{
	case GLX::kMouseDown:
		// handle click
		if (GLX::GetClickFlags(e) & GLX::kClickFlagDbl)
		{
		}
		return true; // trap event

	case K32("MyCustomEvent"):
		// handle custom event
		return true;
	}

	return GLX::Object::OnEvent(src, e); // Always forward to base
}
```

Combined with the dispatch helpers, it's common to use if/else checks instead of a switch:
```cpp
if (e.id == GLX::kMouseDown)
{
	return true;
}
else if (auto menu = GLX::GetMenu(e))
{
	menu->AddItem(L"Option 1");
	return true;
}
```

Always forward unhandled events to the base OnEvent implementation, or delegates and default behavior will not run.

#### Delegate-Based Binding

GLX supports inline delegate binding to avoid subclassing.
```cpp
auto btn = GLX::AddInline(*this, GLX::Init(New<GLX::Button>(L"Click Me"), m_button_style));

GLX::BindClick(btn, []()
{
	// Handle click
});
```

BindEvent and BindEventVoid provide binding to a specific event id, while SetEventDelegate allows full forwarding of all events.
As BindEvent uses the event id also for the delegate id, each usage of BindEvent with the same event id will replace any previous delegate for that event id.  To attach multiple event handlers use SetEventDelegate with different ids.

#### Event Helpers

Some useful helpers for dispatching events include...
```cpp
UInt8 GetClickFlags(const Event & e);   //Returns click state flags.
bool IsLeftClick(const Event & e);      //True if left mouse button was used.
bool IsRightClick(const Event & e);     //True if right mouse button was used.
bool IsDoubleClick(const Event & e);    //True if a double click was detected.
Point GetMouseDelta(const Event & e);   //Mouse drag or wheel delta.
UInt8 GetModifierKeys(const Event & e); //Modifier key flags.
KeyCode GetKeyCode(const Event & e);    //Key code from key events.
WChar GetKeyCharacter(const Event & e);    //Character from key events.
TRef<Menu> GetMenu(Event & e);          //Returns Menu from kMenuOpen event.
TRef<Menu> GetMenu(Event & e, Key32 context); //Same, filtered by context.
Transaction GetTransactionStage(const Event & e); //Retrieve transaction stage.
TRef<Object> GetDragSource(Event & e);  //Retrieve drag source.
```


#### Example: Custom Event End-to-End

```cpp
// Emit a custom event with a property
GLX::Emit(*this, K32("VolumeChanged"), "value", 0.75f);
```

```cpp
// Handle it in a parent view
bool MyView::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (e.id == K32("VolumeChanged"))
	{
		auto v = Data::GetFloat32(e, "value");
		ApplyVolume(v);
		return true;
	}
	return Base::OnEvent(src, e);
}
```


### Typedefs

[KeyCode](#sym-9609741839313765642), [ModifierKeys](#sym-16781546979716072714)

### Globals

[kCharacter](#sym-11040927282821694730), [kFocus](#sym-479710398262641930), [kKeyDown](#sym-13606745366836905226), [kKeyUp](#sym-503732725874787594), [kLoseFocus](#sym-8127020559235122442), [kMouseDown](#sym-11672120847782479114), [kMouseDrag](#sym-11672131731229607178), [kMouseEnter](#sym-16250030719408112906), [kMouseLeave](#sym-16284209515098899722), [kMouseUp](#sym-7921307822078395658), [kMouseWheel](#sym-16340717300300743946), [kTransaction](#sym-9428781797547803914)

### Functions

[BindClick](#sym-16354089260070044938), [BindEvent](#sym-16365802615138780426), [BindEventVoid](#sym-10149938692383409418), [Emit](#sym-8972511126587802890), [EnableMouse](#sym-11035566029764920586), [EnableMouseCapture](#sym-1466049126687933706), [FocusBranch](#sym-6931605747713279242), [GetClickFlags](#sym-877631799612769546), [GetKeyCharacter](#sym-14581210129891427594), [GetKeyCode](#sym-15647254058237003018), [GetModifierKeys](#sym-14527751964743271690), [GetMousePosition](#sym-9233775494737003786), [IsDoubleClick](#sym-5896420424950056202), [IsLeftClick](#sym-5604517198925235466), [IsRightClick](#sym-12740634717330703626), [QueryAntecedent](#sym-16990406935763322122), [RedirectFocus](#sym-15559101598243325194), [ScaleDelta](#sym-9250026225055925514), [Send](#sym-8974635224138876170), [SetEventDelegate](#sym-3610858371567158538), [TransformPosition](#sym-1484818945520272650), [UnbindEvent](#sym-8092100911750546698)

### Enums

[ClickFlags](#sym-1404815326977295626), [TransactionStage](#sym-3412323203366880522)

### Object Types

[Event](#sym-946331961880839434)
---

<a id="sym-5816385031260828211"></a>

##### [Reflex](#sym-14880872606059205893) > [GLX](#sym-830891113689873099) > [Events](#sym-12782211053022971146) > Drag & Drop

GLX drag and drop passes a Reflex::Object payload through the event system. The payload can be any Object, including GLX::Object, Data::PropertySet, ObjectOf <TYPE>, or any custom object-type.
```cpp
auto payload = Make<Data::PropertySet>();
Data::SetWString(payload, "path", L"example.txt");
GLX::StartDragDrop(payload, GLX::kMouseCursorPointer, GLX::kMouseCursorBlock);
```


#### Initiating a drag

To receive kMouseDrag and kMouseUp events on an object, first enable mouse capture:
```cpp
GLX::EnableMouseCapture(object, true);
```

The typical pattern is to watch kMouseDrag and use ExceedsDragThreshold(GetMouseDelta(e)) to decide when the pointer movement is large enough to begin a drag operation.
```cpp
bool MyView::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (e.id == GLX::kMouseDrag)
	{
		if (GLX::ExceedsDragThreshold(GLX::GetMouseDelta(e)))
		{
			GLX::StartDragDrop(Make<MyDragData>(), GLX::kMouseCursorPointer, GLX::kMouseCursorBlock);
			return true;
		}
	}

	return Base::OnEvent(src, e);
}
```


#### Accepting drops

A potential drop target becomes active by handling kDragDropTender and returning true. Once a target accepts kDragDropTender, it will then receive kDragDropEnter, kDragDropLeave, and kDragDropReceive events for that drag.
```cpp
bool MyView::OnEvent(GLX::Object & src, GLX::Event & e)
{
	switch (e.id.value)
	{
	case GLX::kDragDropTender:
		return GLX::QueryDragDropData<MyDragData>(e);

	case GLX::kDragDropEnter:
		src.SetState("dragover");
		return true;

	case GLX::kDragDropLeave:
		src.ClearState("dragover");
		return true;

	case GLX::kDragDropReceive:
		if (auto data = GLX::QueryDragDropData<MyDragData>(e))
		{
			ConsumeDrop(*data);
			return true;
		}
		return false;
	}

	return Base::OnEvent(src, e);
}
```

If you want to prevent this object and its parents from accepting the current drag, you can intercept the tender event, clear its id, and then allow it to continue upward as a non-drag event:
```cpp
case GLX::kDragDropTender:
	e.id = kNullKey;	//change the Event id to effectively hide the event from parents
	return false;	//return false as this object doesnt want to receive the drop
```


#### Reading drag data

QueryDragDropData<TYPE>(e) is the standard typed helper. Internally it performs a DynamicCast on the object stored in the event's drag payload.
```cpp
struct CustomDragData : public Reflex::Object
{
	REFLEX_OBJECT(CustomDragData, Reflex::Object);

	Array <WString> filepaths;
};

template <class TYPE> inline TYPE * Reflex::GLX::QueryDragDropData(GLX::Event & e)
{
	return DynamicCast<TYPE>(GetDragDropData(e));
}
```

If you use a custom payload type, make it object-castable with REFLEX_OBJECT so DynamicCast can recognise it.
When checking against multiple possible payload types, it is slightly more efficient to fetch the payload once and then test it manually:
```cpp
auto drag_data = GLX::GetDragDropData(e);

if (auto custom = DynamicCast<CustomDragData>(drag_data))
{
}
else if (auto generic = DynamicCast<Data::PropertySet>(drag_data))
{
}
```


#### Cursor feedback

The drag API takes two cursors: the cursor shown while hovering an accepting target, and the cursor shown when the current target does not accept the drag. A simple setup is:
```cpp
GLX::StartDragDrop(data, GLX::kMouseCursorPointer, GLX::kMouseCursorBlock);
```

For richer UI, a common pattern is to hide the OS drag cursor and render your own drag-preview object in the window foreground. Start the drag with invisible cursors:
```cpp
GLX::StartDragDrop(data, GLX::kMouseCursorInvisible, GLX::kMouseCursorInvisible);
```

A typical custom-cursor implementation uses a global begin-listener to construct the preview from the drag payload type, attaches that object to the window foreground, disables mouse handling on it, and updates its position every frame.
A key rule is to exclude the visual cursor from hit-testing. Use GLX::EnableMouse(cursor, false, true) so it ignores mouse-over events.
```cpp
m_drag_drop_visualiser = GLX::CreateDragDropBeginListener([this](Reflex::Object & drag_data)
{
	auto origin = GLX::Core::desktop->GetMouseOver();
	TRef <GLX::WindowClient> window = origin->GetWindow();

	//create drag cursor
	auto cursor = New<GLX::Object>();
	GLX::SetText(cursor, GLX::GetText(origin));
	cursor->SetStyle(GLX::FindStyle(origin, "DragCursor"));
	GLX::EnableMouse(cursor, false, true);	//ensure cursor, or any children on cursor, do not doesnt intercept mouse (see EnableMouse for details)
	GLX::Enter(cursor, GLX::kEnterAnimationFade);
	GLX::AddAbsolute(window->GetForeground(), cursor, window->GetMousePosition());

	//attach callbacks to window, so if window is destroyed they wont be called
	SetAbstractProperty(window, "dragdrop_clock", GLX::CreateAnimationClock([window, cursor](Float)
	{
		cursor->SetPosition(window->GetMousePosition());
	}));
	auto run_opacity_animation = [cursor](GLX::Object & drop_target)
	{
		auto fade = GLX::CreateOpacityAnimation("opacity", 0.75f, 0.9f);
		if (IsNull(drop_target)) fade->Flip();
		GLX::Run(cursor, "opacity", 0.25f, fade);
	};
	run_opacity_animation(Null<GLX::Object>());	//call now to apply initial fade to cursor

	SetAbstractProperty(window, "dragdrop_target", GLX::CreateDragDropTargetListener([this, run_opacity_animation](GLX::Object & drop_target)
	{
		m_drop_target.Load()->ClearState("dragover");	//using Reflex::Detail::WeakRef to cover case drop_target might be stack-based
		m_drop_target.Store(drop_target);
		drop_target.SetState("dragover");

		run_opacity_animation(drop_target);
	}));
	SetAbstractProperty(window, "dragdrop_end", GLX::CreateDragDropEndListener([this, window, cursor]()
	{
		GLX::Exit(cursor, true);	//detach cursor with fade

		m_drop_target.Clear();

		UnsetAbstractProperty(window, "dragdrop_clock");
		UnsetAbstractProperty(window, "dragdrop_target");
		UnsetAbstractProperty(window, "dragdrop_end");	//important always delete containing lambda last
	}));
});
```

The above is stil a fairly basic solution, if you support multiple payload types, a convenient pattern is to register a small factory per drag-data type and choose the preview object in CreateDragDropBeginListener. This lets file drags, object drags, and generic property payloads each supply their own visual cursor
In a multi-instance plugin environment, take care to do this once, otherwise each window would create a mousecursor.

#### Notes

- The drag payload is any Reflex::Object; Data::PropertySet is a convenient generic choice.
- kDragDropTender is the gatekeeper event. If it does not return true, the target will not receive the rest of the drag lifecycle.
- The 'examples/Drag & Drop Demo' project shows a complete working pattern, including moving UI objects between rows and restoring the source when the drop is not accepted.


### Globals

[kDragDropEnter](#sym-7745368244894139658), [kDragDropLeave](#sym-7779547040584926474), [kDragDropReceive](#sym-1266180577300677898), [kDragDropReceiveExternal](#sym-7557535242487050), [kDragDropTender](#sym-18263983963024491786)

### Functions

[CancelDragDrop](#sym-17000498287842788618), [CreateDragDropBeginListener](#sym-656786913419691274), [CreateDragDropEndListener](#sym-14306163233164199178), [CreateDragDropTargetListener](#sym-8221730798634632458), [ExceedsDragThreshold](#sym-4967958786020050186), [GetDragDropData](#sym-1676901685567522058), [QueryDragDropData](#sym-12831746998201845002), [StartDragDrop](#sym-7415551474236753162)
---

<a id="sym-13854936142299890954"></a>

##### [Reflex](#sym-14880872606059205893) > [GLX](#sym-830891113689873099) > Layout

GLX layout is handled by a layout model attached to each Object. By default every Object uses the standard layout model (kStandardLayout), which supports common “box layout” behaviors: inline flow, inline flex, float (pin), stretch (fill), plus absolute positioning.
You typically don’t write layout code directly. Instead:
- Set the parent’s flow (how it arranges inline children) via SetFlow.
- Set a positioning mode for each child (how the parent treats that child) via the AddXXX or EnableXXX helpers.
- Optionally enable auto-fit on the parent (size-to-content).

A useful mental model:
- Parents control *flow*.
- Children control *positioning*.


#### Layout Models

Each Object owns a layout model (GLX::Detail::LayoutModel), which can be changed via Object::SetLayoutModel.
The following standard models are provided:
- kStandardLayout: single-line inline flow, plus float/overlay, and absolute positioning.
- kStandardLayoutWrapped: identical behavior, but inline children wrap onto new rows/columns when space runs out.

Custom layout models can be implemented by deriving from GLX::Detail::LayoutModel, but this is a secondary API. Most UIs should use the standard model and helpers described below.

#### Parent Flow

The parent controls how *inline* children are arranged. This includes the primary axis, optional inversion, and optional centering of the inline group.
- GLX::SetFlow(parent, GLX::kFlowX): inline children flow left -> right.
- GLX::SetFlow(parent, GLX::kFlowY): inline children flow top -> bottom.
- GLX::kFlowInvert: reverse the flow direction.
- GLX::kFlowCenter: center the inline group along the primary axis.

```cpp
auto panel = New<GLX::Object>();
GLX::SetFlow(panel, GLX::kFlowY);                        //vertical stack
//GLX::SetFlow(panel, GLX::kFlowY | GLX::kFlowCenter);   //stack centered as a group
```

Flow only affects children in *inline* mode. Float and absolute children ignore flow entirely.

#### Auto-fit (size to content)

Auto-fit is enabled by default. This means an object will grow its content size to accommodate its children.
A child’s contribution to content size is derived from:
- Its style size / max properties.
- Its render layers (e.g. Text layers naturally size to text).
- Its own children, if auto-fit is enabled on that child.

```cpp
GLX::EnableAutoFit(panel, true, true);		//fit width + height to children
//GLX::EnableAutoFit(panel, true, false);	//fit width only
```

Auto-fit is commonly disabled on large scrolling containers, but useful for popups, pills, menus, and small panels.

#### Child Positioning Modes

Each child has a positioning mode that is interpreted by its parent’s layout model.
In the standard model, this is set either:
- When adding the child via AddInline / AddFloat / AddAbsolute
- Explicitly via EnableInline / EnableFloat / EnableAbsolute.

The AddXXX helpers are convenience functions that:
- Configure the child’s positioning mode (via the EnableXXX helpers)
- Attach the child to the parent (via Object::SetParent).

As positioning mode and position are child-properties, they persist when changing the parent:
```cpp
auto parent = New<GLX::Object>();
auto another_parent = New<GLX::Object>();
auto child = New<GLX::Object>();

GLX::AddFloat(parent, child, GLX::kAlignmentTopRight);

child->Detach();
child->SetParent(another_parent); //child is still pinned top-right on the new parent

GLX::EnableFloat(child, GLX::kAlignmentBottomLeft);	//now moved to bottom left
```


### Inline Positioning

Inline children participate in the parent’s flow. They are laid out sequentially along the flow axis.
The Orientation parameter controls placement on the *orthogonal axis* (perpendicular to flow):
- kOrientationNear: near edge (top or left)
- kOrientationCenter: centered on the cross axis
- kOrientationFar: far edge (bottom or right)
- kOrientationFit: stretch to available cross-axis space (default)

```cpp
auto row = New<GLX::Object>();
GLX::SetFlow(row, GLX::kFlowX);

auto icon = New<GLX::Object>();
auto label = New<GLX::Label>(L"Settings");

GLX::AddInline(row, icon, GLX::kOrientationCenter);  //y centered (ortho axis) on row
GLX::AddInline(row, label);                          //fit to row height
```


### Inline Flex

Using flex allows the child to expand along the *primary axis* to consume remaining space.
Conceptually similar to “flex: 1”.
```cpp
auto header = New<GLX::Object>();
GLX::SetFlow(header, GLX::kFlowX);

auto title = New<GLX::Label>(L"Project");
auto spacer = New<GLX::Object>();
auto close = New<GLX::Button>(L"X");

GLX::AddInline(header, title, GLX::kOrientationCenter);
GLX::AddInlineFlex(header, spacer);
GLX::AddInline(header, close, GLX::kOrientationCenter);
```

The remaining space is distributed across all children with flex enabled evenly.
To achieve an exact 50% / 50% horizontal split, enable flex on both children and disable horizontal auto-fit so their size is not influenced by content:
```cpp
auto container = New<GLX::Object>();
GLX::SetFlow(header, GLX::kFlowX);

auto left = New<GLX::Label>(L"Left");
auto right = New<GLX::Button>(L"Right");

GLX::EnableAutoFit(left, false, true);    //Important: prevent content from affecting width
GLX::EnableAutoFit(right, false, true);

GLX::AddInlineFlex(container, left);
GLX::AddInlineFlex(container, right);
```


### Float Positioning

Float positions a child relative to the parent’s bounds but removes it from inline flow.  Floated children do not affect layout of inline siblings.
Float positioning can be specified using:
- Alignment enum (single value)
- Two orientations (x, y)

```cpp
auto view = New<GLX::Object>();

auto badge = New<GLX::Label>(L"NEW");
GLX::AddFloat(view, badge, GLX::kAlignmentTopRight);

auto spinner = New<GLX::Object>();
GLX::AddFloat(view, spinner, GLX::kAlignmentCenter);

auto sticky_footer = New<GLX::Object>();
GLX::AddFloat(view, sticky_footer, GLX::kOrientationFit, GLX::kOrientationFar);	//note use of 2nd variant with Orientation enum
```

Orientation values in float mode mean:
- Near: anchor to start edge
- Center: center on that axis
- Far: anchor to end edge
- Fit: stretch along that axis


### Stretch (fill parent)

Stretch is a convenience mode for the most common float case: filling the parent on both axes.
It is shorthand for enabling float with kOrientationFit on X and Y.
```cpp
GLX::AddStretch(parent, child);	//same as GLX::AddFloat(parent, child, GLX::kOrientationFit, GLX::kOrientationFit)
```


### Absolute (explicit coordinates)

Absolute places a child at an explicit position in the parent’s coordinate space.
```cpp
auto canvas = New<GLX::Object>();
auto node = New<GLX::Object>();
GLX::AddAbsolute(canvas, node, {120.0f, 80.0f});
```

Genuine use-cases for absolute positioning are very rare, and should generally be avoided.
The majority of cases where you may think you need absolute positioning are likely covered by use of GLX::Scroller / GLX::Zoomable or custom drawing.
Furthermore, typical genuine cases for absolute positioning will require a custom layout model, to ensure the child positions are updated in response to the parent size (during the Align phase)

#### Standard Layout Variants

When used correctly the standard layout model will cover most use cases in typical UI layouts.  The additional standard layout variants cover most remaining use cases:

### kStandardLayoutWrapped

To allow inline children to wrap, set the parent’s layout model to kStandardLayoutWrapped.
Child APIs remain unchanged.
Typical use cases:
- Tag strips / pill lists
- Wrapping toolbars
- Flow-style grids without custom layout code


### Typedefs

[Point](#sym-1001298644097402122), [Rect](#sym-8974479385545508106), [Size](#sym-8974655638118434058)

### Functions

[AddAbsolute](#sym-7316673660566209802), [AddFloat](#sym-8691317006378566922), [AddInline](#sym-10626316634991723786), [AddInlineFlex](#sym-5056430349198918922), [AddStretch](#sym-1346689314062435594), [BranchContains](#sym-12012218215368328458), [CalculateAbs](#sym-14943435499698750730), [CalculateAbsoluteRect](#sym-12186597521128260874), [CalculateRelativeRect](#sym-14502939204234872074), [EnableAutoFit](#sym-17128905620876002570), [GetBounds](#sym-18017645576891499786), [LookupBranchIndex](#sym-3350518443588814090), [LookupChildAtIndex](#sym-14192400093637870858), [LookupIndex](#sym-13692084125035496714), [QueryChildById](#sym-13513791923680281866), [QueryElementById](#sym-12767373087771559178), [SetFlow](#sym-17065738407277331722)

### Enums

[Alignment](#sym-13575353154517697802), [FlowFlags](#sym-17523243275744544010), [Orientation](#sym-429781442101347594)

### Object Types

[Object](#sym-14361920786363680010), [WindowClient](#sym-14450129931335730442)

### Value Types

[Range](#sym-1009347082097624330)
---

<a id="sym-1171591279885255946"></a>

##### [Reflex](#sym-14880872606059205893) > [GLX](#sym-830891113689873099) > Styling

GLX supports a powerful visual styling system designed to clearly separate presentation from layout. Layout logic is implemented in C++ or Reflex VM code, while stylesheets remain no-code, making them accessible to designers without impacting development.
Stylesheets avoid the unpredictable inheritance rules of CSS while still enabling reuse via includes, states, and the inherit: property.
The unique bg/fg layer concept allows highly rich designs to be expressed almost entirely through vector primitives. With a wide set of predefined render layers, you can build fully resizable, vectorised UIs that eliminate reliance on pre-baked bitmaps and bloated binaries. Layers support advanced anti-aliasing for crisp visuals at any resolution.
Dynamic property binding (&property) further extends styles into the runtime, powering fluid, data-driven animations and live effects.

#### Stylesheets

Load a stylesheet from disk or resources using RetrieveStyleSheet.
A StyleSheet is itself a Style, so you typically apply it to your root view.
```cpp
auto sheet = GLX::RetrieveStyleSheet(L":res:MyApp/styles.txt");
view->SetStyle(sheet);
```

Recommended: use Bootstrap::SetStyleSheet for hot-reload during development.
```cpp
Bootstrap::SetStyleSheet(view, L":res:MyApp/styles.txt");
```


#### Style lifecycle and OnSetStyle

Whenever a style is set or hot-reloaded, the framework calls OnSetStyle(const Style & style) on that object.
This is the right place to style sub-objects so they also update on hot-reload.
```cpp
void MyApp::View::OnSetStyle(const GLX::Style & style)
{
	auto header_style = style["Header"];
	auto tab_style = header_style["Tab"];

	m_header->SetStyle(header_style);
	for (auto & tab : m_header) tab->SetStyle(tab_style);
}
```


#### Common Properties

Styles support a core set of properties that define sizing, spacing, colors, layers, and state transitions.

### Margin properties

The 'margin' and 'padding' properties accept 1, 2, or 4 float values.
By default, values use Reflex ordering:
```cpp
margin: 24;           // all edges
margin: 16,8;         // width, height
margin: 12,8,12,8;    // left, top, right, bottom
```

You can opt into CSS-style ordering:
```cpp
#option margin_syntax css
margin: 24;           // all edges
margin: 16,8;         // vertical, horizontal
margin: 12,8,12,8;    // top, right, bottom, left
```

Margins define spacing outside the object's bounds, while padding defines spacing inside, around its content.
While margin and padding are set on Style's, most layers also accept an 'indent' property. e.g:
```cpp
bg: Fill(indent: 8,12; corner: 4);
```


### Size properties

'size' and 'max' accept 1 or 2 floats:
```cpp
size: 64;        // 64 x 64
size: 200,48;    // width 200, height 48
max: 400,200;    // maximum size
```

'size' sets the minimum content size, while 'max' constrains it.

### Color properties

The 'color' (or 'colour') property sets the "pen" colour, whereas 'bg_color' (or 'bg_colour') defines a rectangular fill colour.  Colors always use 0-255 ranges for components, including the alpha channel.
```cpp
colour: 128;                // greyscale
colour: 128,128;            // greyscale + alpha
colour: 255,128,0;          // RGB
bg_colour: 0,128,255,192;   // RGBA
```

You can also set colour via hex codes:
```cpp
colour: $FF0000;
```

These map to normalized (0.0f - 1.0f) floats.
Most layers also support a colour property, which is multiplied by the "pen" colour.
```cpp
bg: Border(width: 2; corner: 4; colour: 255,0,0);
```


### Opacity

The 'opacity' property is a normalized single float value (0.0 - 1.0), applied multiplicatively to all layers and children of an object.
```cpp
opacity: 0.75;
```


### Layers

Each object supports two layer arrays: 'bg' (drawn before children) and 'fg' (drawn after children). Typically use bg, but for cases where rendering "on top of" content is required, use fg (e.g. InnerShadow).
Both properties take an array of layers. Layers can be freely combined, but the draw order is strictly in the order specified.
```cpp
bg: Fill(colour: 240), Border(colour: 0, width: 1);
fg: Text(value: &label, font: Small, colour: 0);
```


### Grouping layers

Some layers (such as Mask and Align) are grouping layers that include a 'content' property, which is itself a nested array of layers:
```cpp
bg:
Mask
(
	mask: Fill(corner: 16);

	content:
	Image(source: artwork; fit: cover),
	Border(corner: 16; width: 2);
);
```


### Transition time

The 'transition' property combined with @State selection is the simplest way to do a basic animation. It sets the blending time in seconds when switching between states.
```cpp
transition: 0.25; // quarter second blend
```


#### States

States are style variants that override properties when active. They appear as @State blocks inside a style and are applied in top-to-bottom order, so later states override earlier ones when both are active.
Push and clear states in code with Object::SetState(Key32) and Object::ClearState(Key32). Hover and focus are the only built-in states and are managed by GLX::Object automatically.
```cpp
Button:
{
	transition: 0.25;
	size: 64;

	@State hover:
	{
		bg: Fill(colour: 236);
	};

	@State selected:
	{
		bg: Fill(colour: 80,176,240);
		fg: Text(colour: 255);
	};

	@State inactive:
	{
		fg: Text(colour: 128);
	};
}
```

Nested states let you specify combinations explicitly.
```cpp
Button:
{
	@State inactive:
	{
		@State selected:
		{
			bg: Fill(colour: 96,120,140);
		};
	};
}
```

The Select, SelectChildren, SelectBranch helper functions set/unset the "selected" id. Activate(object, false) pushes "inactive" and disables input on that object.

#### Includes and Resources

Stylesheets can include other sheets for reuse and structure.
```cpp
include: ["common.txt", "controls.txt"];
```

Stylesheets can define resources for use in layers like Text, Image, TextEdit.
```cpp
@Font app_font:
{
	path: ":res:MyApp/fonts/Inter-Regular.ttf";
	size: 14;
};

@Bitmap logo:
{
	path: ":res:MyApp/bitmaps/logo.png"; // png,bmp supported; jpg mostly supported
};

Header:
{
	fg: Text(font: app_font; colour: 32; value: &title);
	bg: Image(source: logo; fit: contain);
}
```

Resources can be declared at the root of the sheet for global access or nested inside a style for specificity.

#### Inheritance and Aliasing

Stylesheets support reuse and composition through the inherit property and the special @Alias directive.
Using inherit, a style copies all properties from another style before applying its own overrides. This allows you to define a base style and then extend or specialize it with minimal duplication:
```cpp
BigButton:
{
	inherit: Button;
	size: 128;
};
```

The inherit property accepts either a single style ID, or a path to a nested sub-style. For example, to inherit from a sub-style defined inside another style:
```cpp
inherit: Dialog > Header;
```

Additionally, the @Alias style variant lets you reuse a style as a named property of another style. Unlike inherit, aliasing does not copy values, and instead directly references the source definition:
```cpp
Popup:
{
	inherit: Button;
	@Alias menu; // alias a menu property defined previously
};
```

This makes aliasing useful when you want multiple styles to share the same reference (for example, to the same menu definition or resource) rather than duplicating properties.

#### Dynamic Binding

Any style property value can bind to a code-side variable by using &name. This allows live updates and animations without reapplying styles.
Commonly used for Text value or colours.
```cpp
// Stylesheet
Flashing:
{
	size: 64;
	bg: Fill(corner: 12; colour: &dynamic_colour);
}
```

```cpp
// C++
View::View()
{
	m_flashing = GLX::Init(New<GLX::Object>(), style["Flashing"]);
	SetProperty(m_flashing, "dynamic_color", New<GLX::ColorObject>(GLX::kWhite));
	GLX::AddFloat(*this, m_flashing, GLX::kAlignmentCenter);
	EnableOnClock(); // enable OnClock(delta) callback
}

void View::OnSetStyle(const GLX::Style & style)
{
	m_flashing->SetStyle(style["Flashing"]);
}

void View::OnClock(Float delta_seconds)
{
	auto colour = QueryProperty<GLX::ColorObject>(m_flashing, "dynamic_color");
	colour->r = /* update channel over time */;
	colour->g = /* update channel over time */;
	colour->b = /* update channel over time */;
	m_flashing->Redraw(); // trigger layer repaint
}
```


#### Patterns and tips

- Use OnSetStyle to style sub-objects so hot-reload propagates automatically.
- Order @State blocks from least to most dominant: hover - selected - inactive. Use nested states for combinations.
- Prefer binding (&name) for any property you want to animate or update at runtime.
- Use hierarchical lookup style["Section"]["Title"] instead of duplicating properties.
- Put common primitives in included sheets to keep app sheets small and focused.


### Typedefs

[Colour](#sym-12411471395882632458), [MouseCursor](#sym-3088564088943510794), [Points](#sym-14596111566169539850)

### Functions

[Activate](#sym-6013620891227882762), [ActivateBranch](#sym-13371338634544907530), [ClearText](#sym-7183523781695079690), [FindStyle](#sym-13118240072967226634), [GetClip](#sym-5843754414980039946), [GetOpacity](#sym-2672623865267586314), [GetText](#sym-5846347737708201226), [IsActive](#sym-991493259530437898), [IsSelected](#sym-12390270548251346186), [RGB](#sym-830941759893767434), [Rescale](#sym-11518256371707512074), [RetrieveStyleSheet](#sym-14510779649853850890), [Rotate](#sym-14933918998927082762), [Select](#sym-15049850890779460874), [SelectBranch](#sym-6899141017635751178), [SelectChildren](#sym-5123920649244476682), [SetBounds](#sym-8061677790082204938), [SetClip](#sym-17065274482089886986), [SetOnStyle](#sym-5762298410296247562), [SetOpacity](#sym-6167080227332785418), [SetState](#sym-9834429827883566346), [SetText](#sym-17067867804818048266), [ToggleState](#sym-14286032820232094986), [Translate](#sym-10667000377695634698), [UnsetBounds](#sym-18234720160403522826), [UnsetClip](#sym-12399408254382343434), [UnsetOpacity](#sym-9836085121164346634)

### Object Types

[Style](#sym-1017425348645717258), [StyleSheet](#sym-7196502683237319946)

### Value Types

[Margin](#sym-14021901793341048074)
---

<a id="sym-12340501116277972008"></a>

##### [Reflex](#sym-14880872606059205893) > [GLX](#sym-830891113689873099) > [Styling](#sym-1171591279885255946) > Canvas

Reflex GLX provides three canvas entry points for custom, code-driven drawing, ordered from simplest and fastest to most advanced:
- SetCanvas for monochrome geometry, using Array<Point> output.
- SetColourCanvas for multi-colour geometry, using Array<ColourPoint> output.
- SetGraphicCanvas for advanced cases where you need direct access to graphics, images, fonts, or a custom Graphic that can change every frame.

In stylesheets, these correspond to:
- bg: Canvas();
- bg: ColourCanvas();
- bg: GraphicCanvas();


#### Execution model

These canvas callbacks are not immediate-mode draw calls in the traditional sense. They are not invoked every frame just because the object is visible.
Instead, the callback prepares geometry for the renderer, effectively building VBO data that can be reused until something changes.
To request a new canvas build, call Redraw() on the object. For example:
```cpp
m_object.Redraw();
```

Other state changes that already imply a visual refresh, such as Update(), Realign(), and related layout-reaccommodation paths, also schedule a redraw.
For animated content, a typical pattern is to trigger redraws from OnClock(Float):
```cpp
void MyView::OnClock(Float dt)
{
	Redraw();
}
```


#### Choosing the right API

Prefer Canvas over ColourCanvas over GraphicCanvas, in that order, for performance and simplicity.
Use Canvas() when you are drawing simple monochrome paths and shapes.
Use ColourCanvas() when your geometry needs per-point colour.
Use GraphicCanvas() only when you need the extra power geometry plus textures (eg image/font) composition (which are non primary APIs).

#### Style usage

Canvas bindings are usually declared in styles with an optional id. Use the plain form when there is only one binding for the object:
```cpp
bg: Canvas();
```

When you have multiple canvas bindings on the same object, give each one an id and bind them explicitly in code:
```cpp
bg: Canvas(id: logo), Canvas(id: clouds);

SetCanvas(m_obj, {.id = K32("logo")}, draw_logo);
SetCanvas(m_obj, {.id = K32("clouds")}, draw_clouds);
```

The same pattern applies to colour and graphic canvases:
```cpp
SetColourCanvas(m_obj, {}, draw_coloured_geometry);
SetGraphicCanvas(m_obj, {}, draw_advanced_graphic);
```

For Canvas and ColourCanvas, do not clear ctx.output. Append to it only. The system may batch multiple canvases together and compact them into a single VBO.

#### Simple Demo

A minimal Canvas demo can draw a star shape by appending a closed path to ctx.output:
```cpp
void StarView::OnSetStyle(const GLX::Style & style)
{
	GLX::SetCanvas(*this, {}, [](GLX::CanvasContext & ctx)
	{
		auto cx = ctx.size.w * 0.5f;
		auto cy = ctx.size.h * 0.5f;
		auto outer = Min(ctx.size.w, ctx.size.h) * 0.42f;
		auto inner = outer * 0.45f;
		GLX::Point star[10];

		for (UInt i = 0; i < 10; i++)
		{
			auto a = -kPif * 0.5f + kPif * 0.2f * Float(i);
			auto r = (i & 1) ? inner : outer;
			star[i] = { cx + Cos(a) * r, cy + Sin(a) * r };
		}

		GLX::AddPath(ctx.output, star, true, 2.0f, GLX::kPathJoinRound, GLX::kPathCapRound);
	});
}
```

This demo uses SetCanvas because the geometry is monochrome. As with most layers, it will be drawn at the style colour, modulated by its own colour property.  Use ColourCanvas only when you need per-point colour, and keep GraphicCanvas for the advanced cases where you need the full Graphic pipeline.

#### Example project

See examples/Custom Drawing for a practical demonstration of the Canvas API in use.

### Typedefs

[ColourPoints](#sym-3902131130752664842)

### Functions

[AddDottedLine](#sym-4770899402942416138), [AddEllipseFill](#sym-12312432654082475274), [AddEllipseOutline](#sym-4928174929158964490), [AddPath](#sym-9208741917075014922), [AddPointsWithColour](#sym-12894796375077782794), [AddPolygonFill](#sym-8690098717430220042), [AddRectFill](#sym-15549464018048615690), [AddRectOutline](#sym-8956172967914603786), [AddRoundedFill](#sym-8495876898985968906), [AddRoundedOutline](#sym-906192056525882634), [AddRoundedTriangleFill](#sym-18110915687986857226), [AddRoundedTriangleOutline](#sym-10591907727838119178), [AddTriangleFill](#sym-17532202238221583626), [AddTriangleOutline](#sym-2847234583865427210), [SetCanvas](#sym-8157410691259663626), [SetColourCanvas](#sym-11451435876106732810), [SetGraphicCanvas](#sym-13078888775147422986), [UnsetCanvas](#sym-18330453061580981514)

### Value Types

[CanvasContext](#sym-6841480785334207754), [ColourCanvasContext](#sym-16969424285355574538), [GraphicCanvasContext](#sym-4995297399615358218)
---

<a id="sym-830948585109412904"></a>

##### [Reflex](#sym-14880872606059205893) > [GLX](#sym-830891113689873099) > [Styling](#sym-1171591279885255946) > SVG

Reflex provides native SVG support, treating vector assets as first-class resources alongside bitmaps. SVGs can be imported and used in stylesheets via the @SVG resource declaration, or decoded in code for custom drawing via the geometry pipeline.

#### SVGs in Stylesheets

SVG usage in stylesheets is consistent with @Bitmap resources and the Image layer. Declare an SVG resource, then reference it in a style property.

### Importing an SVG

Declare an SVG resource using the @SVG type:
```cpp
@SVG my_svg_id:
{
	path: "assets/some.svg";
};
```

Then use it as an Image source:
```cpp
bg: Image(source: my_svg_id; fit: contain; anchor: center);
```


### SVG Size

@SVG also supports a size property. If the SVG file contains intrinsic dimensions, those will be used by default. However, some SVGs do not specify a size, in which case you must set one explicitly using standard size syntax:
```cpp
@SVG my_svg_id:
{
	path: "assets/some.svg";
	size: 128;
};
```

Because SVGs are vector-based, size does not affect rendering quality in the way it does for bitmaps. However, the initial size is important as it affects the initial geometry preparation, including corner steps and other tessellation details.

### Multi-Icon SVG Sets

Reflex supports icon sets where multiple icons are defined in a single SVG file using the '<symbol>' tag with unique IDs. Declare the SVG resource once, then select individual icons using the '>' frame selector - the same syntax used for bitmap sprite frames.
```cpp
@SVG icons:
{
	path: "assets/icons.svg";
};
```

Reference a specific icon by its symbol ID:
```cpp
bg: Image(source: icons > circle_icon; fit: contain; anchor: center);
```

As per-standard stylesheet syntax, if the symbol ID contains characters that are not a single token (such as a dash), wrap it in single quotes:
```cpp
bg: Image(source: icons > 'circle-icon'; fit: contain; anchor: center);
```


#### Advanced Usage: SVG in Code

The examples/SVG Demo project demonstrates parsing an SVG file and coupling it with custom drawing. The basic workflow is:
- Load the SVG file into memory.
- Decode it using the XML decoder into a PropertySet.
- Use InspectSVG and DecodeSVG to convert it into geometry suitable for a VBO.
- Render using SetColourCanvas (as DecodeSVG produces Array<GLX::ColourPoint>).


### C++ Example

```cpp
void ViewImpl::OnUpdate()
{
	auto xml_bytes = File::Open(m_path_to_svg);
	auto xml = Make<Data::PropertySet>(Data::DecodePropertySet(Data::kReflexXmlFormat, xml_bytes));

	if (auto svgs = GLX::Detail::InspectSVG(xml))
	{
		GLX::SetColourCanvas(m_icon, {}, {[xml, svg = svgs.GetFirst()](GLX::ColourCanvasContext & ctx)
		{
			GLX::Detail::DecodeSVG(ctx.output, svg, size);
		}});
	}
	else
	{
		GLX::UnsetCanvas(m_icon, {});
	}
}

void ViewImpl::OnSetStyle(const GLX::Style & style)
{
	m_icon.SetStyle(style["Icon"]);
}
```

Note: In the example, capturing 'xml' as a Reference (produced by Make) in the lambda is critical to keep the PropertySet alive for the lifetime of the canvas binding.
Due to the complexity of the SVG format, decoding is expensive and should be avoided per-frame. In the example, the size_z guard in the example ensures the geometry is only re-decoded when the target size actually changes.

### Stylesheet for Code-Driven SVG

```cpp
Icon:
{
	bg: ColourCanvas();		//use Canvas() for monochrome geometry, ColourCanvas() for colour-point geometry
};
```


#### Summary

- Use @SVG to declare SVG resources, with the same Image layer syntax as @Bitmap.
- Set size explicitly for SVGs that lack intrinsic dimensions; initial size affects geometry preparation.
- Use the > frame selector with symbol IDs to reference individual icons from multi-icon SVG files.
- Single-quote symbol IDs that contain dashes or other non-token characters.
- For custom drawing, decode via the XML pipeline and use InspectSVG / DecodeSVG to produce ColourPoint geometry.
- Render code-driven SVGs with SetColourCanvas.

---

<a id="sym-2955407527103268106"></a>

##### [Reflex](#sym-14880872606059205893) > [GLX](#sym-830891113689873099) > Widgets

Widgets are the higher-level interactive controls built on top of GLX::Object and a small set of reusable behaviours.
Most of them are event-driven composites rather than standalone rendering systems: they emit requests such as select, open, load, remove, and transaction updates, and they usually apply named substyles to internal parts such as header, body, footer, item, tab, prev, or next.
A recurring pattern in this module is that visual feedback comes from ordinary GLX state flags such as selected, hover, open, and reorder, so stylesheet state handling is a big part of how these controls are customised.
Many of the classes here are also intended to be assembled together. Form, Selector, Button, and Menu all act as building blocks for larger widgets rather than being isolated controls.

### Functions

[CloseContextMenu](#sym-13586872535978444042), [OpenContextMenu](#sym-9476589778599249162)

### Enums

[SelectionMode](#sym-832412837035859918)

### Object Types

[AbstractList](#sym-13581836222967416074), [AbstractViewBar](#sym-8104147582051255562), [AbstractViewPort](#sym-9184679921144923402), [Form](#sym-8972676074806805770), [Label](#sym-978729750598092042), [List](#sym-8973574272727483658), [Menu](#sym-8973709207715022090), [Popup](#sym-1001332359590675722), [RangeBar](#sym-6607440668686124298), [RotarySlider](#sym-3660065068848088330), [Scroller](#sym-16992810420937000202), [Selector](#sym-8578895112124335370), [Split](#sym-1016746791057589514), [TabGroup](#sym-1584402070130230538), [TextArea](#sym-9244439658014803210), [VirtualList](#sym-7391239356591211786), [Zoomable](#sym-17739790527466931466)
---

<a id="sym-8974499447388207819"></a>

##### [Reflex](#sym-14880872606059205893) > SIMD

The Reflex::SIMD namespace provides a high-performance, cross-platform abstraction for Single Instruction, Multiple Data (SIMD) operations. It allows for data-parallel processing of 4-element vectors using platform-specific hardware acceleration (such as SSE, AVX, or NEON) through a unified C++ interface.

#### Core Types

The library centers around the TypeV4<T> template, which represents a vector of four elements. Specialized typedefs are provided for common data types:
- FloatV4: A vector of 4 Float32 values.
- IntV4: A vector of 4 Int32 values.
- BoolV4: A mask type (using kBooleanTrue or kBooleanFalse) used for conditional SIMD logic.


#### Initialization and Access

Vectors can be initialized via broadcasting a single value, specifying four distinct values, or loading from memory.
```cpp
using namespace Reflex::SIMD;

FloatV4 a(1.0f);                    // Broadcast: [1.0, 1.0, 1.0, 1.0]
FloatV4 b(1.0f, 2.0f, 3.0f, 4.0f);   // Explicit:  [1.0, 2.0, 3.0, 4.0]

Float32 data[4] = { 5.0f, 6.0f, 7.0f, 8.0f };
FloatV4 c = LoadUnaligned(data);     // Load from memory
```

Accessing individual elements can be done via the [] operator or by retrieving a Quad<T> structure. Use GetFirst() to efficiently grab the element at index 0.

#### Mathematical Operations

Standard arithmetic operators are overloaded to perform component-wise operations across all four lanes simultaneously.
- Arithmetic: +, -, *, / and MulAdd(a, b, c) (computes a * b + c).
- Comparison: ==, !=, >, >=, <, <= (returns a BoolV4 mask).
- Common Math: Abs, Sign, Min, Max, SquareRoot, Reciprocal.
- Advanced: Exp2, Log2, and LinearInterpolate(t, a, b).


#### Conditional Logic (Masking)

Because SIMD lanes cannot branch independently, conditional logic is handled via masking and the Select function.
```cpp
// Choose between 'a' and 'b' based on a condition
BoolV4 mask = a > b;
FloatV4 result = Select(mask, a, b); // result[i] = mask[i] ? a[i] : b[i]
```

You can also analyze the state of a mask using these helpers:
- Any(mask): Returns true if at least one lane is true.
- Full(mask): Returns true if all four lanes are true.
- Empty(mask): Returns true if no lanes are true.
- Count(mask): Returns the number of true lanes (0-4).


#### Shuffling and Transposition

The library provides tools to reorder data within vectors or between two different vectors.
```cpp
// Reorder elements within a vector
auto swapped = Shuffle<1, 0, 3, 2>(v);

// Interleave values from two vectors
auto lo = InterleaveLo(a, b);

// Transpose a 4x4 matrix represented by four vectors
Transpose(row0, row1, row2, row3);
```


#### Horizontal Operations

While most operations are vertical (lane-to-lane), some operations combine values across a single vector:
```cpp
Float32 total = Sum(my_v4); // Adds all 4 components into a single scalar
```


#### Conversion

Explicit conversion between integer and floating-point vectors is required:
- ToFloatV4(IntV4): Converts integers to floats.
- ToIntV4(FloatV4): Converts floats to integers (rounding).
- Truncate(FloatV4): Converts floats to integers using truncation toward zero.


### Typedefs

[BoolV4](#sym-12243830513440697580), [FloatV4](#sym-1452735807989787884), [IntV4](#sym-965537622992137452)

### Functions

[Abs](#sym-830866282021170412), [And](#sym-830867918403710188), [Any](#sym-830868008598023404), [ClipNormal](#sym-13508490827818118380), [Count](#sym-935139373479406828), [Empty](#sym-944995146602687724), [Exp](#sym-830888096160066796), [Exp2](#sym-8972563282258341100), [Full](#sym-8972703281012321516), [GetFlags](#sym-8391697121869868268), [GetFree](#sym-5844244906422873324), [Invert](#sym-13416385593457814764), [Log](#sym-830919522435771628), [Log2](#sym-8973600349356600556), [Max](#sym-830922288394710252), [Min](#sym-830923379316403436), [Modulo](#sym-14091115712889793772), [Not](#sym-830928932709117164), [Or](#sym-25179805120507116), [Pow](#sym-830938300032789740), [Reciprocal](#sym-8345292607281736940), [RoundDown](#sym-16767473181223595244), [RoundNearest](#sym-8124602026378702060), [Select](#sym-15049850888367156460), [SelectNot](#sym-6402223207565798636), [Sign](#sym-8974652981416340716), [SquareRoot](#sym-2751346404323011820), [operator!=](#sym-6112104527567564012), [operator&](#sym-4657153260484340972), [operator*](#sym-4657153277664210156), [operator+](#sym-4657153281959177452), [operator-](#sym-4657153290549112044), [operator/](#sym-4657153299139046636), [operator<](#sym-4657153354973621484), [operator<=](#sym-6112108354383424748), [operator==](#sym-6112108496117345516), [operator>](#sym-4657153363563556076), [operator>=](#sym-6112108637851266284), [operator|](#sym-4657153629851528428)

### Enums

[Boolean](#sym-16664858372343377132)

### Value Types

[TypeV4](#sym-15320421235171044588)
---

<a id="sym-15152871578414578379"></a>

##### [Reflex](#sym-14880872606059205893) > System

Reflex is designed so that application code is completely platform-agnostic. When developing with Reflex, there is no need to interact with OS-specific APIs directly - all system functionality is abstracted and implemented in a fully cross-platform way.
The System namespace is responsible for this abstraction layer.  It provides the low-level, platform-independent primitives that Reflex builds upon.

#### Design intent

The System namespace is not intended to be the primary API used by application code.
Instead:
- Higher-level namespaces (Data, File, GLX etc.) are built on top of System.
- These higher-level APIs provide safer, more convenient, and more expressive interfaces.
- System exists to unify OS behavior and expose consistent primitives across all supported platforms.


#### Typical usage

In most cases, you should use the higher-level APIs rather than System directly.
For example, instead of working with System::FileHandle, use the File namespace for more convenient helper functions,
```cpp
//Low-level (usually unnecessary)
auto file_handle = Make<System::FileHandle>(path);
Array <UInt8> bytes1(file_handle.GetSize());
auto bytes_read = file_handle->Read(bytes1.GetData(), bytes1.GetSize());
bytes1.SetSize(bytes_read);

//Recommended
auto bytes2 = File::Open(path);
```


#### When to use System directly

Direct use of the System namespace is appropriate in advanced or specialized cases, such as:
- When you need lower-level control than higher-level APIs expose.
- When implementing new subsystems or extending Reflex itself.
- When performance, lifetime, or resource management must be handled explicitly.

In these cases, System provides a stable, cross-platform foundation without leaking OS-specific concepts into application code.

#### Custom Implementations

As most System APIs are defined as pure-abstract interfaces, the System layer acts as an extensibility point as well as an OS abstraction. This allows you to provide alternate implementations (for example, a FileHandle that reads from an in-memory blob, a cloud stream, or a virtual file system) and pass them directly to other APIs operating on the System primitives.

### Typedefs

[ColourPoint](#sym-7385145563652677668), [fPoint](#sym-18136990298152230948), [fRect](#sym-1108859096084373540), [fSize](#sym-1109035348657299492)

### Functions

[Delete](#sym-12528574736920338468), [Exists](#sym-12793038812755386404), [GetElapsedTime](#sym-12080007777000480804), [GetNumProcessor](#sym-13432161900956868644), [GetOperatingSystemVersion](#sym-13467897282990039076), [GetPath](#sym-5845711014251847716), [GetSystemID](#sym-6411672276618567716), [GetTime](#sym-5846364819943448612), [IsDirectory](#sym-15415839623751000100), [MakeDirectory](#sym-4686556266557988900), [Open](#sym-8974068044792979492), [Rename](#sym-14882056995100287012)

### Enums

[KeyCode](#sym-9609741836169109540), [ModifierKeys](#sym-16781546976571416612), [MouseCursor](#sym-3088564085798854692), [Path](#sym-8974154334980923428), [Response](#sym-11235966723864114280)

### Object Types

[DirectoryIterator](#sym-6040881140000219172), [DiskIterator](#sym-13914065361941159972), [DynamicLibrary](#sym-10482214990098063396), [FileHandle](#sym-15071417447436378148), [HttpConnection](#sym-5746192168122441764), [Process](#sym-2589384810356138020), [Renderer](#sym-10332891952312344612), [Renderer::Canvas](#sym-5663778785163599908), [Renderer::Graphic](#sym-8968105079702937636), [Task](#sym-8974771599090769956), [Thread](#sym-15234142333668810788), [Window](#sym-15742871520433791012)

### Value Types

[Colour](#sym-12411471392737976356), [ReceiveDataFn](#sym-454116815258760296), [ReceiveHeaderFn](#sym-9794357931599705192)
---


##### API Reference


##### 

---


##### Reflex

<a id="sym-17531698610406914763"></a>

#### Reflex::CString

```cpp
using CString = [Array <char>](#sym-925399584115751627);
```

<a id="sym-2022350079943473867"></a>

#### Reflex::CString::View

```cpp
using CString::View = [ArrayView <char>](#sym-17111432006044187339);
```

<a id="sym-974353891938499275"></a>

#### Reflex::Key32

Key32 is Reflex's 32-bit identifier type. It is used extensively throughout the framework anywhere a compact, comparable id is needed.
Common uses include property ids, property addresses, GLX event ids, style ids, and other lightweight keys used to identify behavior or data without storing strings at runtime.
When constructed from a string literal or const char source in normal code, the hash is typically produced at compile time via the consteval constructor path. This makes Key32 convenient to write and very cheap to compare.
```cpp
Data::SetFloat32(object, "gain", 0.75f);
if (e.id == MakeKey32("MyEvent"))
{
}
auto button_style = style["Button"];
```

Key32 is also useful when you need to branch on received text without doing repeated string comparisons:
```cpp
switch (MakeKey32(some_received_string))
{
case MakeKey32("one"):
	break;

case MakeKey32("two"):
	break;
}
```

Because Key32 stores only the hash value, reverse lookup to the original text is not available unless you explicitly maintain that mapping elsewhere, for example in a KeyMap.
```cpp
using Key32 = [Key <UInt32>](#sym-830913507649354443);
```

<a id="sym-17787487981880441547"></a>

#### Reflex::WString

```cpp
using WString = [Array <WChar>](#sym-925399584115751627);
```

<a id="sym-5727895666471172811"></a>

#### Reflex::WString::View

```cpp
using WString::View = [ArrayView <WChar>](#sym-17111432006044187339);
```

<a id="sym-18172314581995846347"></a>

#### Reflex::AcquireProperty

```cpp
[Reference <TYPE>](#sym-1695362219560368843) AcquireProperty([Object](#sym-14361920786414140107)& object, [Key32](#sym-974353891938499275) id);
[Reference <TYPE>](#sym-1695362219560368843) AcquireProperty([Object](#sym-14361920786414140107)& object, [Key32](#sym-974353891938499275) id, VARGS...);
```

<a id="sym-8453966097060952779"></a>

#### Reflex::AutoRelease

```cpp
[Reference <TYPE>](#sym-1695362219560368843) AutoRelease(TYPE& object);
```

<a id="sym-13647811632263559883"></a>

#### Reflex::GetAbstractProperty

```cpp
[TRef <Object>](#sym-8974699438245378763) GetAbstractProperty([Object](#sym-14361920786414140107)& object, [Key32](#sym-974353891938499275) id);
[ConstTRef <Object>](#sym-532369501975706315) GetAbstractProperty(const [Object](#sym-14361920786414140107)& object, [Key32](#sym-974353891938499275) id);
```

<a id="sym-8052652234187700939"></a>

#### Reflex::GetProperty

```cpp
[ConstTRef <TYPE>](#sym-532369501975706315) GetProperty([Object](#sym-14361920786414140107)& object, [Key32](#sym-974353891938499275) id);
```

<a id="sym-8973292196505811659"></a>

#### Reflex::Join

```cpp
[Array <TYPE>](#sym-925399584115751627) Join(const VARGS...& ...);
```

Varadic function concatenating 'args' returning an Array.
- Input args can be Array <TYPE>, ArrayView <TYPE> or TYPE.
- In the case of CString / WString, null terminated const char * can also be passed as an argument.

<a id="sym-8973553721359432395"></a>

#### Reflex::Left

```cpp
[ArrayView <TYPE>](#sym-17111432006044187339) Left(const [ArrayView <TYPE>](#sym-17111432006044187339)& view, [UInt32](#sym-15243775351508400843) position);
```

<a id="sym-12955598882812951243"></a>

#### Reflex::Lowercase

```cpp
void Lowercase(const TYPE& string);
char Lowercase(char character);
[WChar](#sym-1030155236624530123) Lowercase([WChar](#sym-1030155236624530123) character);
```

<a id="sym-8973690004966701771"></a>

#### Reflex::Make

```cpp
[Reference <TYPE>](#sym-1695362219560368843) Make(VARGS... args);
```

<a id="sym-984515724431293131"></a>

#### Reflex::Merge

```cpp
void Merge(const [ArrayView <TYPE>](#sym-17111432006044187339)& view, const [ArrayView <TYPE>](#sym-17111432006044187339)& delimiter);
```

<a id="sym-830923338829494987"></a>

#### Reflex::Mid

```cpp
[ArrayView <TYPE>](#sym-17111432006044187339) Mid(const [ArrayView <TYPE>](#sym-17111432006044187339)& view, [UInt32](#sym-15243775351508400843) position);
[ArrayView <TYPE>](#sym-17111432006044187339) Mid(const [ArrayView <TYPE>](#sym-17111432006044187339)& view, [UInt32](#sym-15243775351508400843) position, [UInt32](#sym-15243775351508400843) length);
```

<a id="sym-830927530717575883"></a>

#### Reflex::New

```cpp
[TRef <TYPE>](#sym-8974699438245378763) New(VARGS... args);
```

<a id="sym-5441740889099827915"></a>

#### Reflex::RawStringCopy

```cpp
[UInt32](#sym-15243775351508400843) RawStringCopy(const TYPE* from, TYPE* to, [UInt32](#sym-15243775351508400843) capacity);
```

<a id="sym-6112432979064355531"></a>

#### Reflex::RawStringLength

```cpp
[UInt32](#sym-15243775351508400843) RawStringLength(const TYPE* string);
[UInt32](#sym-15243775351508400843) RawStringLength(const TYPE* string, [UInt32](#sym-15243775351508400843) capacity);
```

<a id="sym-14881969406732368587"></a>

#### Reflex::Remove

```cpp
void Remove([Array <TYPE>](#sym-925399584115751627)& array, const TYPE& element_or_array);
```

<a id="sym-11504363754578213579"></a>

#### Reflex::Replace

```cpp
[Array <TYPE>](#sym-925399584115751627) Replace(const [ArrayView <TYPE>](#sym-17111432006044187339)& view, const TYPE& element_or_array);
```

<a id="sym-7135086472021113547"></a>

#### Reflex::ReverseSearch

```cpp
[Idx](#sym-830904007181695691) ReverseSearch(const [ArrayView <TYPE>](#sym-17111432006044187339)& view, const TYPE& element_or_array);
```

<a id="sym-7192770605815659211"></a>

#### Reflex::ReverseSplice

```cpp
[Pair < ArrayView <TYPE> , ArrayView <TYPE> >](#sym-8974152822052584139) ReverseSplice(const [ArrayView <TYPE>](#sym-17111432006044187339)& view, [UInt32](#sym-15243775351508400843) position);
```

<a id="sym-1010549333688548043"></a>

#### Reflex::Right

```cpp
[ArrayView <TYPE>](#sym-17111432006044187339) Right(const [ArrayView <TYPE>](#sym-17111432006044187339)& view, [UInt32](#sym-15243775351508400843) position);
```

<a id="sym-15048213812505443019"></a>

#### Reflex::Search

```cpp
[Idx](#sym-830904007181695691) Search(const [ArrayView <TYPE>](#sym-17111432006044187339)& view, const TYPE& element_or_array);
```

<a id="sym-550441268363360971"></a>

#### Reflex::SetAbstractProperty

```cpp
void SetAbstractProperty([Object](#sym-14361920786414140107)& object, [Key32](#sym-974353891938499275) id, [TRef <Object>](#sym-8974699438245378763) property);
```

<a id="sym-15105897946299988683"></a>

#### Reflex::Splice

```cpp
[Pair < ArrayView <TYPE> , ArrayView <TYPE> >](#sym-8974152822052584139) Splice(const [ArrayView <TYPE>](#sym-17111432006044187339)& view, [UInt32](#sym-15243775351508400843) position);
```

<a id="sym-1016746791108049611"></a>

#### Reflex::Split

```cpp
[Array < ArrayView <TYPE> >](#sym-925399584115751627) Split(const [ArrayView <TYPE>](#sym-17111432006044187339)& view, const TYPE& delimiter);
```

<a id="sym-10018937018765245131"></a>

#### Reflex::ToCString

```cpp
[CString](#sym-17531698610406914763) ToCString(const [WString::View](#sym-5727895666471172811)& string);
[CString](#sym-17531698610406914763) ToCString(const [WString](#sym-17787487981880441547)& string);
[CString](#sym-17531698610406914763) ToCString([UInt32](#sym-15243775351508400843) value);
[CString](#sym-17531698610406914763) ToCString([UInt64](#sym-15243775785300097739) value);
[CString](#sym-17531698610406914763) ToCString([Int32](#sym-965532656177740491) value);
[CString](#sym-17531698610406914763) ToCString([Int64](#sym-965533089969437387) value);
[CString](#sym-17531698610406914763) ToCString([Float32](#sym-1452730841175390923) value, [UInt32](#sym-15243775351508400843) precision, bool discard_zeros);
[CString](#sym-17531698610406914763) ToCString([Float64](#sym-1452731274967087819) value, [UInt32](#sym-15243775351508400843) precision, bool discard_zeros);
```

Converts a value to a CString.
- string: Source string to convert.
- value: Numeric value to convert.
- precision: Number of digits after the decimal point (floating-point overloads).
- discard_zeros: If true, trailing fractional zeros are removed.
- return: Resulting CString.

Supported conversions include:
- WString and WString::View -> CString
- Signed and unsigned integers -> CString
- Floating-point values -> CString

Floating-point overloads format the value using the specified precision and optional trailing-zero trimming.
<a id="sym-12386713323243272907"></a>

#### Reflex::ToFloat32

```cpp
[Float32](#sym-1452730841175390923) ToFloat32(const [CString::View](#sym-2022350079943473867)& string);
[Float32](#sym-1452730841175390923) ToFloat32(const [WString::View](#sym-5727895666471172811)& string);
```

Converts a string to a Float32.
- string: Source string to convert.
- return: Parsed 32-bit floating-point value.

Supported conversions include:
- CString::View -> Float32
- WString::View -> Float32

The string is interpreted as a base-10 floating-point value. Behaviour is undefined if the string does not contain a valid numeric representation.
<a id="sym-12386713757034969803"></a>

#### Reflex::ToFloat64

```cpp
[Float64](#sym-1452731274967087819) ToFloat64(const [CString::View](#sym-2022350079943473867)& string);
[Float64](#sym-1452731274967087819) ToFloat64(const [WString::View](#sym-5727895666471172811)& string);
```

Converts a string to a Float32.
- string: Source string to convert.
- return: Parsed 64-bit floating-point value.

Supported conversions include:
- CString::View -> Float64
- WString::View -> Float64

The string is interpreted as a base-10 floating-point value. Behaviour is undefined if the string does not contain a valid numeric representation.
<a id="sym-5633841749614099147"></a>

#### Reflex::ToInt32

```cpp
[Int32](#sym-965532656177740491) ToInt32(const [CString::View](#sym-2022350079943473867)& string);
[Int32](#sym-965532656177740491) ToInt32(const [WString::View](#sym-5727895666471172811)& string);
```

<a id="sym-5633842183405796043"></a>

#### Reflex::ToInt64

```cpp
[Int64](#sym-965533089969437387) ToInt64(const [CString::View](#sym-2022350079943473867)& string);
[Int64](#sym-965533089969437387) ToInt64(const [WString::View](#sym-5727895666471172811)& string);
```

<a id="sym-2914517720508889803"></a>

#### Reflex::ToRegion

```cpp
[ArrayRegion <TYPE>](#sym-2445480886326753995) ToRegion(const [ArrayRegion <TYPE>](#sym-2445480886326753995)& value);
[ArrayRegion <TYPE>](#sym-2445480886326753995) ToRegion([Allocation <TYPE>](#sym-6570175900388915915)& allocation);
[ArrayRegion <TYPE>](#sym-2445480886326753995) ToRegion([Array <TYPE>](#sym-925399584115751627)& value);
```

<a id="sym-3277278771522271947"></a>

#### Reflex::ToUInt32

```cpp
[UInt32](#sym-15243775351508400843) ToUInt32(const [CString::View](#sym-2022350079943473867)& string);
[UInt32](#sym-15243775351508400843) ToUInt32(const [WString::View](#sym-5727895666471172811)& string);
```

Converts a string to a UInt32.
- string: Source string to convert.
- return: Parsed unsigned 32-bit integer value.

Supported conversions include:
- CString::View -> UInt32
- WString::View -> UInt32

The string is interpreted as a base-10 integer. Behaviour is undefined if the string does not contain a valid unsigned integer representation.
<a id="sym-3277279205313968843"></a>

#### Reflex::ToUInt64

```cpp
[UInt64](#sym-15243775785300097739) ToUInt64(const [CString::View](#sym-2022350079943473867)& string);
[UInt64](#sym-15243775785300097739) ToUInt64(const [WString::View](#sym-5727895666471172811)& string);
```

Converts a string to a UInt32.
- string: Source string to convert.
- return: Parsed unsigned 64-bit integer value.

Supported conversions include:
- CString::View -> UInt64
- WString::View -> UInt64

The string is interpreted as a base-10 integer. Behaviour is undefined if the string does not contain a valid unsigned integer representation.
<a id="sym-15265494386943949515"></a>

#### Reflex::ToView

```cpp
[ArrayView <TYPE>](#sym-17111432006044187339) ToView(const [ArrayView <TYPE>](#sym-17111432006044187339)& value);
[ArrayView <TYPE>](#sym-17111432006044187339) ToView(const [ArrayRegion <TYPE>](#sym-2445480886326753995)& value);
[ArrayView <TYPE>](#sym-17111432006044187339) ToView(const TYPE& value);
[ArrayView <TYPE>](#sym-17111432006044187339) ToView(const [Allocation <TYPE>](#sym-6570175900388915915)& allocation);
[ArrayView <TYPE>](#sym-17111432006044187339) ToView(const [Array <TYPE>](#sym-925399584115751627)& value);
```

<a id="sym-10274726390238771915"></a>

#### Reflex::ToWString

```cpp
[WString](#sym-17787487981880441547) ToWString(const [CString::View](#sym-2022350079943473867)& string);
[WString](#sym-17787487981880441547) ToWString(const [CString](#sym-17531698610406914763)& string);
[WString](#sym-17787487981880441547) ToWString([UInt32](#sym-15243775351508400843) value);
[WString](#sym-17787487981880441547) ToWString([UInt64](#sym-15243775785300097739) value);
[WString](#sym-17787487981880441547) ToWString([Int32](#sym-965532656177740491) value);
[WString](#sym-17787487981880441547) ToWString([Int64](#sym-965533089969437387) value);
[WString](#sym-17787487981880441547) ToWString([Float32](#sym-1452730841175390923) value, [UInt32](#sym-15243775351508400843) precision, bool discard_zeros);
[WString](#sym-17787487981880441547) ToWString([Float64](#sym-1452731274967087819) value, [UInt32](#sym-15243775351508400843) precision, bool discard_zeros);
```

Converts a value to a WString.
- string: Source string to convert.
- value: Numeric value to convert.
- precision: Number of digits after the decimal point (floating-point overloads).
- discard_zeros: If true, trailing fractional zeros are removed.
- return: Resulting WString.

Supported conversions include:
- CString and CString::View -> WString
- Signed and unsigned integers -> WString
- Floating-point values -> WString

Floating-point overloads format the value using the specified precision and optional trailing-zero trimming.
<a id="sym-17365949766550387403"></a>

#### Reflex::UnsetAbstractProperty

```cpp
void UnsetAbstractProperty([Object](#sym-14361920786414140107)& object, [Key32](#sym-974353891938499275) id);
```

<a id="sym-11355172447734430411"></a>

#### Reflex::Uppercase

```cpp
void Uppercase(const TYPE& string);
char Uppercase(char character);
[WChar](#sym-1030155236624530123) Uppercase([WChar](#sym-1030155236624530123) character);
```

<a id="sym-9214008371664091851"></a>

#### Reflex::Address

Reflex::Address is the internal key used to identify a property address. It combines the property id and property type into a single value.
You do not usually use it directly in normal code, because the type portion is typically selected implicitly via the property template or typed accessor you call.
Lower-level object property callbacks such as Object::OnSetProperty receive a Reflex::Address to describe which typed property is being accessed.
```cpp
[Key32](#sym-974353891938499275) Address::id;
```

```cpp
[UInt32](#sym-15243775351508400843) Address::type_id;
```

<a id="sym-925399584115751627"></a>

#### template Reflex::Array <TYPE>

Type-safe dynamic contiguous container.
Array stores elements in contiguous memory with explicit size and capacity management. It supports value types and object types, with optimized paths for raw-copyable items.

### Storage and Allocation

Array owns its storage through an Allocator reference.
- Capacity grows on demand via Allocate(...) or automatically via push/append operations.
- Over-allocation mode is supported to reduce reallocations during growth.
- Compact() reallocates to current size.

Clear() destroys active elements and keeps capacity. The destructor clears elements and releases owned memory.

### Size and Mutation

Array supports SetSize(...), Expand(...), Shrink(...), Push(...), Pop(), Insert(...), Remove(...), and Append(...).
For non-trivial element types, constructors and destructors are called correctly during growth, shrink, and relocation.
For raw-constructible/copyable element types, operations use bulk memory paths for performance.

### Null-Terminated Mode

If TYPE is marked null-terminated, Array keeps an additional trailing terminator element that is maintained after each mutation.
This mode enables string-like usage from the same container type while preserving explicit size.

### Access and Iteration

Array provides indexed access, first/last accessors, direct data pointer access, and forward/reverse iteration.
Bounds are validated with assertions for index-based operations.

### Usage Example

```cpp
Array<int> values;
values.Push(10);
values.Push(20);
values.Insert(1, 15);
values.Remove(0);

for (auto & v : values)
{
	// v = 15, then 20
}
```

```cpp
void Array::Allocate([UInt32](#sym-15243775351508400843) capacity);
```

```cpp
[UInt32](#sym-15243775351508400843) Array::GetCapacity();
```

```cpp
void Array::Compact();
```

```cpp
void Array::Clear();
```

```cpp
TYPE& Array::Push();
TYPE& Array::Push(const TYPE& value);
```

```cpp
void Array::Pop();
```

```cpp
TYPE& Array::Insert([UInt32](#sym-15243775351508400843) idx);
TYPE& Array::Insert([UInt32](#sym-15243775351508400843) idx, const TYPE& value);
```

```cpp
void Array::SetSize([UInt32](#sym-15243775351508400843) size);
```

```cpp
[UInt32](#sym-15243775351508400843) Array::GetSize();
```

```cpp
TYPE& Array::operator[]([UInt32](#sym-15243775351508400843) idx);
const TYPE& Array::operator[]([UInt32](#sym-15243775351508400843) idx);
```

```cpp
bool Array::operator bool();
```

```cpp
TYPE* Array::GetData();
const TYPE* Array::GetData();
```

```cpp
void Array::Append(const [ArrayView <TYPE>](#sym-17111432006044187339)& values);
```

```cpp
bool Array::operator<(const [ArrayView <TYPE>](#sym-17111432006044187339)& value);
```

```cpp
bool Array::operator==(const [ArrayView <TYPE>](#sym-17111432006044187339)& value);
```

```cpp
bool Array::operator!=(const [ArrayView <TYPE>](#sym-17111432006044187339)& value);
```

<a id="sym-2445480886326753995"></a>

#### template Reflex::ArrayRegion <TYPE>

```cpp
TYPE* ArrayRegion::data;
```

```cpp
[UInt32](#sym-15243775351508400843) ArrayRegion::size;
```

<a id="sym-17111432006044187339"></a>

#### template Reflex::ArrayView <TYPE>

```cpp
const TYPE* ArrayView::data;
```

```cpp
[UInt32](#sym-15243775351508400843) ArrayView::size;
```

<a id="sym-16417332563359791819"></a>

#### template Reflex::ConstReference <TYPE>

<a id="sym-532369501975706315"></a>

#### template Reflex::ConstTRef <TYPE>

<a id="sym-1452730841175390923"></a>

#### Reflex::Float32

<a id="sym-1452731274967087819"></a>

#### Reflex::Float64

<a id="sym-5470074934380427979"></a>

#### template Reflex::Function <RTN(VARGS...)>

Type-erased callable wrapper with semantics similar to std::function, but always safe to invoke.
An empty Function is implicitly bound to a null function that returns a default-constructed value. As a result, calling an unset Function is well-defined and never undefined behavior.
When the return type is not default-constructible, the function must be bound at construction and cannot exist in the empty (null-bound) state. This is enforced at compile time.
Clear() resets the function to the same null-bound state. This operation is likewise unavailable for non-default-constructible return types.
The function converts to bool, evaluating to true when a callable is bound and false when in the null-bound state.
```cpp
Function <Int32(Int32)> fn;

output.Log(True(fn));     //logs false

auto v = fn(10);             //safe, returns default int (0)

fn = [](Int32 x)
{
	return x * 2;
};

output.Log(True(fn));    //logs true

fn.Clear();                 //resets to null function
```

```cpp
void Function::Clear();
```

```cpp
RTN Function::Invoke(VARGS...);
```

```cpp
RTN Function::operator()(VARGS...);
```

<a id="sym-830904007181695691"></a>

#### Reflex::Idx

Lightweight integral index type used to represent either a valid position within an indexable container or an invalid/unset state.
Idx is typically used as the return type for search and lookup operations. A special sentinel value represents an invalid or not-found index.
- Integral type, cheap to copy and compare.
- Intended for indexing into contiguous or indexable data structures.
- Casts to bool to indicate set/valid (true) or unset/invalid (false).

```cpp
if (Idx idx = Search(array, value))
{
	auto & item = array[idx.value];
}
```

```cpp
[UInt32](#sym-15243775351508400843) Idx::value;
```

```cpp
bool Idx::operator bool();
```

<a id="sym-965532389889768139"></a>

#### Reflex::Int16

<a id="sym-965532656177740491"></a>

#### Reflex::Int32

<a id="sym-965533089969437387"></a>

#### Reflex::Int64

<a id="sym-8973134498191604427"></a>

#### Reflex::Int8

<a id="sym-8973160663132371659"></a>

#### template Reflex::Item <TYPE>

Intrusive list node base type.
Item provides link storage and membership operations for objects that participate in List.

### Membership Model

An Item belongs to at most one List at a time.
Attaching an item automatically detaches it from any current list before linking into the target list.
Destroying an attached item unlinks it safely from its list.

### Positioning APIs

Item supports:
- Attach(list) to append at the end.
- InsertBefore(item) and InsertAfter(item) for relative insertion.
- SendBottom() and SendTop() for reordering within the same list.
- SetIndex(idx) to move to a target list position.
- Detach() to remove from current list.

GetPrev(), GetNext(), and GetList() expose neighborhood and ownership context.

### Lifecycle Hooks

OnAttach() and OnDetach() can be implemented by derived item types.
These hooks are invoked during normal attach/detach operations and allow custom side effects.

### Usage Example

```cpp
struct Entry : public Item<Entry>
{
	using Item::Attach;
	using Item::Detach;
	using Item::InsertBefore;

	void OnAttach() { /* became linked */ }
	void OnDetach() { /* became unlinked */ }
};

List<Entry> entries;
Entry x, y;
x.Attach(entries);
y.InsertBefore(x);
y.Detach();
```

```cpp
[List <TYPE>](#sym-8973574272777943755)* Item::GetList();
const [List <TYPE>](#sym-8973574272777943755)* Item::GetList();
```

```cpp
[Item <TYPE>](#sym-8973160663132371659)* Item::GetPrev();
const [Item <TYPE>](#sym-8973160663132371659)* Item::GetPrev();
```

```cpp
[Item <TYPE>](#sym-8973160663132371659)* Item::GetNext();
const [Item <TYPE>](#sym-8973160663132371659)* Item::GetNext();
```

<a id="sym-830913507649354443"></a>

#### template Reflex::Key <TYPE>

```cpp
TYPE Key::value;
```

<a id="sym-8973574272777943755"></a>

#### template Reflex::List <TYPE>

Intrusive doubly-linked list container.
List manages ordering and traversal of items that derive from Item<TYPE,...>. The list does not allocate per-node wrappers; link fields live inside each item.

### Ownership and Retention

Item can be configured with RETAIN = true or false.
- With RETAIN = true, attaching an item retains it and detaching releases it.
- With RETAIN = false, list linkage is non-retaining.
- List itself stores links only and does not create or destroy item objects.


### Core Operations

List exposes GetFirst(), GetLast(), GetNumItem(), Empty(), and boolean conversion.
Item-side APIs (Attach/InsertBefore/InsertAfter/Detach) update list links and counts atomically with respect to list invariants.
List destruction clears membership by detaching all linked items.

### Iteration and Safety

Forward and reverse iterators are provided.
In debug builds, iterator dereference validates modification state to catch invalid iteration across structural mutation.
SafeIterate(...) is available for mutation-tolerant traversal patterns.
```cpp
[UInt32](#sym-15243775351508400843) List::GetNumItem();
```

```cpp
bool List::Empty();
```

```cpp
[Item <TYPE>](#sym-8973160663132371659)* List::GetFirst();
const [Item <TYPE>](#sym-8973160663132371659)* List::GetFirst();
```

```cpp
[Item <TYPE>](#sym-8973160663132371659)* List::GetLast();
const [Item <TYPE>](#sym-8973160663132371659)* List::GetLast();
```

<a id="sym-830922256497736395"></a>

#### template Reflex::Map <KEY,VALUE>

```cpp
void Map::Clear();
```

```cpp
TYPE1* Map::Search(const TYPE1& key, TYPE2* fallback);
const TYPE2* Map::Search(const TYPE1& key, const TYPE2* fallback);
```

```cpp
TYPE2& Map::Set(const TYPE1& key, const TYPE2& value);
```

```cpp
void Map::Unset(const TYPE1& key);
```

```cpp
TYPE2& Map::operator[](const TYPE1& idx);
const TYPE2& Map::operator[](const TYPE1& idx);
```

<a id="sym-3887337300172356192"></a>

#### State::Monitor

Lightweight observer used to detect changes in a State via polling.
A Monitor is connected to a State and records the last observed change count. Calling Poll() reports whether the associated State has changed since the previous poll.

### Lifetime Management

State::Monitor does not manage the lifetime of the observed State. It is the caller’s responsibility to ensure that the State remains alive for the duration of the Monitor’s lifetime.
For cases where strong lifetime coupling is required, clients would typically wrap a Monitor together with an owning reference to the State’s owner.
```cpp
struct RetainingMonitor
{
	RetainingMonitor(Object & owner, State & owner_state)
		: m_owner(owner)
		, m_monitor(owner_state)
	{
	}
	Reference <Object> m_owner; // retains state owner
	State::Monitor m_monitor;
};
```

```cpp
void Monitor::Connect(const [State](#sym-1017314229302295243)& state);
```

```cpp
void Monitor::Reconnect();
```

```cpp
void Monitor::Disconnect();
```

```cpp
bool Monitor::Poll();
```

<a id="sym-8973908842140367563"></a>

#### template Reflex::Node <TYPE>

Hierarchical intrusive node built from Item + List.
Node combines sibling linkage (as an Item in a parent list) and child ownership/view (as a List of children), enabling tree structures without external wrapper nodes.

### Tree Semantics

Each node can have:
- Zero or one parent (via Item membership).
- Zero or more children (via inherited List interface).

Attach(parent) links a node as a child of parent.
GetParent() returns the parent node or null for roots.

### Traversal Helpers

Node provides parent and branch traversal utilities.
- ParentRange / ConstParentRange iterate upward through ancestors.
- BranchIterator / ConstBranchIterator iterate descendants in depth-first order.

LookupBranchIndex(root, node) returns the top-level child index beneath root that contains node.
BranchContains(parent, child) returns true if child lies in parent's ancestry chain.

### Usage Example

```cpp
struct TreeItem  : public Node<WidgetNode>
{
	using Node::Attach;
};

TreeItem root, child, grandchild;
child.Attach(root);
grandchild.Attach(child);

bool inside = BranchContains(root, grandchild);
```

<a id="sym-12240650208584891083"></a>

#### Reflex::NullType

<a id="sym-15739513838455978699"></a>

#### template Reflex::ObjectOf <TYPE>

ObjectOf<T> wraps a value type inside a Reflex::Object so that it can participate in object-based APIs without designing a dedicated object class.
This is commonly useful when you want to heap-allocate a plain value, store it in a Reference, attach it as a typed property, pass it through generic object channels, or return it from async/task-style APIs.
```cpp
auto offset = Make<ObjectOf<GLX::Point>>();
object.SetProperty("offset", New<ObjectOf<GLX::Point>>({10, 20}));
auto payload = New<ObjectOf<Function<void()>>>(callback);
```

The wrapped value is stored in the public .value member.
A useful detail is that ObjectOf<T> automatically provides a null-instance when T is trivially constructible. This makes wrappers such as ObjectOf<bool>, ObjectOf<UInt32>, ObjectOf<Key32>, and similar simple value types integrate cleanly with Reference<T> and TRef<T> default construction.
<a id="sym-14460294615737268939"></a>

#### Reflex::Output

```cpp
void Output::Log(VARGS... args);
```

```cpp
void Output::Warn(VARGS... args);
```

```cpp
void Output::Error(VARGS... args);
```

<a id="sym-8974152822052584139"></a>

#### template Reflex::Pair <TYPE1,TYPE2>

<a id="sym-1001298644147862219"></a>

#### template Reflex::Point <TYPE>

```cpp
TYPE Point::x;
```

```cpp
TYPE Point::y;
```

<a id="sym-1007300444332194507"></a>

#### template Reflex::Queue <TYPE,SIZE>

Lock-free single-producer single-consumer queue.
Queue is a ring-buffer queue designed for SPSC workloads. It uses atomic read/write positions and does not take locks.

### Concurrency Model

Queue is intended for one producer thread and one consumer thread.
- Producer uses Push(...).
- Consumer uses Pop(...) or Flush(...).
- Coordination is performed with atomic indices.

SIZE must be a power of two.

### Capacity and Wrap

For fixed-size queues, storage is TYPE m_data[SIZE] and index wrap uses bitmasking.
Push(...) fails and returns false when full.
Pop(...) fails and returns false when empty.

### Flush Behavior

Flush(value) reads the latest queued value and advances read position to write position, effectively dropping older pending entries.
Flush() clears all pending entries without returning a value.

### Usage Example

```cpp
Queue<int, 1024> q;

// producer thread
q.Push(42);

// consumer thread
int v = 0;
if (q.Pop(v))
{
	// consumed value
}
```

```cpp
void Queue::Push(const TYPE& value);
void Queue::Push(TYPE temp);
```

```cpp
bool Queue::Pop(TYPE& out);
```

```cpp
bool Queue::Flush(TYPE& last_out);
```

<a id="sym-8974479385595968203"></a>

#### template Reflex::Rect <TYPE>

```cpp
[Point <TYPE>](#sym-1001298644147862219) Rect::origin;
```

```cpp
[Size <TYPE>](#sym-8974655638168894155) Rect::size;
```

<a id="sym-1695362219560368843"></a>

#### template Reflex::Reference <TYPE>

Reference<T> is the primary ownership wrapper for Reflex objects.
It automatically retains on construction and releases on destruction, giving deterministic RAII-style lifetime management while guaranteeing a valid object reference over its lifetime.
Reference<T> default-constructs to the type's null-instance rather than nullptr. If a type does not define a null-instance, Reference<T> cannot be default-constructed.
```cpp
Reference<MyClass> ref = New<MyClass>();
ref->DoSomething();
```

This is broadly equivalent to manual Retain()/Release() management:
```cpp
MyClass * ptr = New<MyClass>();
ptr->Retain();
ptr->DoSomething();
ptr->Release();
```

In practice, Reference<T> is safer, clearer, and strongly preferred over manual retain/release handling.
<a id="sym-9501096024522062539"></a>

#### template Reflex::Sequence <KEY,VALUE>

```cpp
[Idx](#sym-830904007181695691) Sequence::Search(const KEY& key);
```

```cpp
void Sequence::Clear();
```

```cpp
TYPE2* Sequence::SearchValue(const TYPE1& key, TYPE2* fallback);
const TYPE2* Sequence::SearchValue(const TYPE1& key, const TYPE2* fallback);
```

```cpp
TYPE2& Sequence::Set(const TYPE1& key, const TYPE2& value);
```

```cpp
TYPE2& Sequence::Insert(const TYPE1& key, const TYPE2& value);
```

```cpp
TYPE2& Sequence::Acquire(const TYPE1& key, VARGS... ...);
```

```cpp
void Sequence::Remove([UInt32](#sym-15243775351508400843) idx);
void Sequence::Remove([UInt32](#sym-15243775351508400843) idx, [UInt32](#sym-15243775351508400843) n);
```

```cpp
[UInt32](#sym-15243775351508400843) Sequence::GetSize();
```

```cpp
TYPE2& Sequence::operator[]([UInt32](#sym-15243775351508400843) idx);
const TYPE2& Sequence::operator[]([UInt32](#sym-15243775351508400843) idx);
```

<a id="sym-15069494894420785867"></a>

#### template Reflex::Signal <VARGS...>

Type-safe push-based notification primitive.
Signal represents an event that can be emitted with a fixed argument signature. Observers register callbacks (listeners) which are invoked synchronously when the signal is notified.
Signals manage listener registration internally and support multiple independent listeners per signal.

### Lifetime Management

Signal stores listeners in an intrusive list and does not retain them. Listener objects are owned by the client.
CreateListener(...) returns an Object representing a listener handle. As notifications are delivered only while this handle remains alive, the receiver would typically store the received Object in a Reference.  When the handle is destroyed, the listener automatically disconnects and will no longer be called.
This design guarantees that Notify(...) never calls into a listener that has already been destroyed, without requiring the signal to manage listener lifetimes or the client to explicitly disconnect.
Signal and listener lifetimes are fully independent: it is safe for a Signal to be destroyed before its listeners, and for listeners to be destroyed before the Signal.

### Dispatch Safety

At the start of a notification, the current listener list is copied into a temporary dispatch buffer using strong references. This guarantees safe iteration in the presence of re-entrancy and list mutation.
- Listener attachment and removal during Notify(...) is always safe.
- Re-entrant calls to Notify(...) are supported.
- The dispatch set is stable for the duration of the call.

Listeners added while a Notify(...) call is in progress will not receive that notification, and will be invoked starting from the next Notify(...).
Listener removal during dispatch takes effect immediately: if a listener is detached or released such that it is only retained by the internal dispatch buffer, it is skipped and will not be invoked.

### Signal::Mute

Supports temporary muting via Signal::Mute.
If a signal is muted, notifications are suppressed until the mute scope is released. Nested mutes are supported.

### Usage Example

```cpp
struct DataStream
	: public Signal<const Packet &>
{
	using Signal<const Packet &>::CreateListener;

	void Receive(const Packet & packet)
	{
		Notify(packet);
	}
};
```

```cpp
m_client.SetProperty(K32("listener"), data_stream.CreateListener([](const Packet & packet)
{
	// process incoming data
}));

//disconnect
m_client.UnsetProperty<Reflex::Object>(K32("listener"));
```

<a id="sym-8974655638168894155"></a>

#### template Reflex::Size <TYPE>

```cpp
TYPE Size::h;
```

```cpp
TYPE Size::w;
```

<a id="sym-1017314229302295243"></a>

#### Reflex::State

Lightweight change-tracking primitive for pull-based notification.
State represents a mutable condition that can be marked as changed. Observers do not receive callbacks; instead, they poll for changes using a State::Monitor.
State is intended to be used as a base class. Its primary mutation method, Notify(), is protected to enforce strict semantics: only the state owner may publish its own changes. External code can observe state transitions via State::Monitor, but cannot trigger them directly.
```cpp
struct MyObject :
	public Reflex::Object,
	public Reflex::State
{
	void SetSomething()
	{
		State::Notify();
	}
};
```

Each call to Notify() increments an internal change counter. Clients use State::Monitor instances to poll for changes since the last observation.
<a id="sym-8974699438245378763"></a>

#### template Reflex::TRef <TYPE>

TRef<T> is Reflex's lightweight non-owning object reference.
Unlike Reference<T>, TRef<T> does not retain or release. Its role is primarily semantic: it expresses temporary access to a valid object without taking ownership.
Like Reference<T>, TRef<T> binds to the type's null-instance rather than nullptr, so it follows the Reflex convention of valid object references instead of nullable smart pointers.
```cpp
Reference<MyClass> owner = New<MyClass>();
TRef<MyClass> handle = owner;
handle->DoSomething();
```

Typical uses include:
- Return values from Create() factory functions
- Getter return values where ownership remains with the parent object
- Function parameters indicating the receiver will retain the object

<a id="sym-1022631093872065227"></a>

#### template Reflex::Tuple <VARGS...>

```cpp
TYPE1 Tuple::a;
```

```cpp
TYPE2 Tuple::b;
```

```cpp
VARGS... Tuple::c (etc);
```

<a id="sym-15243775085220428491"></a>

#### Reflex::UInt16

<a id="sym-15243775351508400843"></a>

#### Reflex::UInt32

<a id="sym-15243775785300097739"></a>

#### Reflex::UInt64

<a id="sym-1020924849394252491"></a>

#### Reflex::UInt8

<a id="sym-1030155236624530123"></a>

#### Reflex::WChar

<a id="sym-6570175900388915915"></a>

#### template Reflex::Allocation <TYPE>

Inherits from [Object](#sym-14361920786414140107)
Fixed-size contiguous object allocation.
Allocation stores a contiguous block of elements with a size chosen at creation time. Unlike Array, Allocation does not track capacity or support growth. The size can only stay the same or be reduced via Shrink(...).
Because the allocation size cannot increase after creation, Allocation is suitable for sharing fixed-size primitive buffers across multiple threads when callers need stable storage and a stable element count.
This does not provide atomic read/write synchronization. Concurrent access still requires the usual external synchronization when multiple threads may read and write the same elements.
Allocation provides indexed access, direct data pointer access, and begin/end iteration in a compact object-owned buffer.
```cpp
const [UInt32](#sym-15243775351508400843) Allocation::size;
```

```cpp
void Allocation::Shrink([UInt32](#sym-15243775351508400843) n);
```

```cpp
TYPE& Allocation::operator[]([UInt32](#sym-15243775351508400843) idx);
const TYPE& Allocation::operator[]([UInt32](#sym-15243775351508400843) idx);
```

```cpp
TYPE* Allocation::GetData();
const TYPE* Allocation::GetData();
```

<a id="sym-13055918718868224715"></a>

#### Reflex::Allocator

Inherits from [Object](#sym-14361920786414140107)
<a id="sym-14361920786414140107"></a>

#### Reflex::Object

Reflex makes a primary, explicit distinction between object-types and value-types. Object types derive from Reflex::Object, are shared, reference counted, typically heap-based (although they can also be instantiated on the stack), and non-copyable by default.
Value types (non-Objects such as UInt32 or Array) are lightweight stack-based primitives. To promote a value into an object so it can participate in reference counting, heap allocation, dynamic properties, or generic object APIs, use ObjectOf<TYPE>.
Reflex::Object provides two foundational capabilities:
- Deterministic lifetime management via intrusive reference counting
- A generic interface for attaching and querying typed properties dynamically


#### Object Lifetime

Reflex uses an intrusive reference counting model built directly into Object.
Unlike std::shared_ptr, COM, or Objective-C, there are no external control blocks or hidden allocations - the reference count lives inside the object itself.

### Core Principles

- All reference-counted types derive from Object
- The reference count is stored inside the object itself
- Newly created objects always start with a reference count of zero
- Every Release() must therefore correspond to a previous Retain()
- Ownership semantics are primarily expressed through Reference<T> and TRef<T>


### Reference <TYPE>

Reference<T> is the primary ownership wrapper for Reflex objects. It retains on construction, releases on destruction, and guarantees a valid object reference over its lifetime.
See [Reflex::Reference](#sym-1695362219560368843) for full semantics and usage.

### TRef <TYPE>

TRef<T> is the lightweight non-owning counterpart to Reference<T>. It does not retain or release, but still follows the Reflex convention of representing a valid object reference rather than a nullable pointer.
See [Reflex::TRef](#sym-8974699438245378763) for full semantics and usage.

### Object Creation

Reflex objects should never be created with raw new/delete directly.
Instead, object creation is performed through the Reflex helper APIs:
```cpp
auto a = New<MyClass>(args...);
auto b = Make<MyClass>(args...);
```

- New<T>() returns a TRef<T>
- Make<T>() returns a Reference<T>
- If T is abstract, New<T>() automatically resolves through T::Create()

Make<T>() is generally the most convenient form when immediate ownership is required.

### AutoRelease Helpers

AutoRelease() provides a shorthand for constructing temporary Reference<T> wrappers.
The following examples are equivalent:
```cpp
Reference<HttpConnection> conn = HttpConnection::Create(...);
auto conn = AutoRelease(HttpConnection::Create(...));
auto conn = Make<HttpConnection>(...);
```


### Stack-Allocated Objects

Unlike traditional COM-style systems, Reflex objects may be instantiated directly on the stack.
```cpp
Data::PropertySet node;
```

Stack objects ignore Retain() and Release() calls and are destroyed normally via scope lifetime.
This flexibility is useful and efficient for temporaries and embedded members, however ownership must remain explicit: any retained reference must not outlive the stack object it refers to. For example, storing a Reference or TRef to a stack object beyond its lifetime is invalid:
```cpp
Data::PropertySet node;
auto props = New<Data::PropertySet>();
props->SetProperty("child", node); // invalid after node leaves scope
```


### Null Instances

Reference<T> and TRef<T> default construct using the type's null-instance rather than nullptr.
If a type does not define a null-instance, the wrappers cannot be default-constructed, enforcing validity at compile time.
User-defined object types therefore often require explicit construction:
```cpp
Reference <View> view1 = New<View>();
Reference <View> view2 = kNewObject;	//shorthand helper
```

TRef<T> also supports construction using kNoValue for deferred assignment, however this should only be used sparingly.

### Manual Retain/Release

Object exposes Retain() and Release(), however direct usage is uncommon.
If manual lifetime management is required:
- New objects begin with refcount = 0
- Every Release() must correspond to a previous Retain()
- Double-release will assert
- Retaining in constructors and releasing in destructors is the standard pattern


### Circular References

Reflex is not a garbage collected system. Circular references will leak unless explicitly broken:
```cpp
auto a = New<Data::PropertySet>();
auto b = New<Data::PropertySet>();
a->SetProperty("foo", b);
b->SetProperty("foo", a);
```

Some advanced systems inside Reflex (such as Reflex::VM) provide explicit cycle-breaking mechanisms, however application code should generally avoid circular ownership through design.

### Best Practices

- Prefer Reference<T> for ownership
- Use TRef<T> for temporary non-owning references
- Avoid manual Retain()/Release() where possible
- Avoid circular ownership relationships
- Prefer clear ownership hierarchies


### ObjectOf<T>

ObjectOf<T> wraps a value type inside an Object, allowing values to participate in reference counting, dynamic properties, and generic object-based APIs.

#### Generic Properties

Reflex::Object defines a virtual interface for attaching, querying, and removing typed properties dynamically via Object::SetProperty, Object::QueryProperty, and Object::UnsetProperty.
This allows sub-objects and arbitrary typed values to be attached to objects at runtime, reducing the need for deep subclassing and enabling highly data-driven systems.
Properties are identified by both id AND type, rather than by id alone. This complements C++'s strong typing model by allowing multiple typed views of the same conceptual property.
Important: Object Does Not Implement the Interface
Reflex::Object itself does not provide a concrete property implementation. Calling SetProperty() on a derived type which does not implement the property callbacks will silently discard the property.
Typically, dynamic properties are implemented through Data::PropertySet, which fully implements the property system for arbitrary types.
Many primary framework classes (such as GLX::Object) derive from Data::PropertySet and therefore support dynamic properties automatically.
For more information on dynamic properties, see [Data::PropertySet](#sym-13039646562789154425).
```cpp
void Object::RetainSt();
```

```cpp
void Object::ReleaseSt();
```

```cpp
void Object::RetainMt();
```

```cpp
void Object::ReleaseMt();
```

```cpp
void Object::UnsetProperty([Key32](#sym-974353891938499275) id);
```

```cpp
void Object::SetProperty([Key32](#sym-974353891938499275) id, TYPE& property);
```

```cpp
TYPE* Object::QueryProperty([Key32](#sym-974353891938499275) id, TYPE& property);
```

```cpp
[Allocator](#sym-13055918718868224715)* Object::GetAllocator();
```

---


##### [Reflex](#sym-14880872606059205893) > Async

<a id="sym-15124628141771903332"></a>

#### Task::Status

```cpp
kStatusPending
kStausFailed
kStatusCompleted
```

<a id="sym-11982832383702358589"></a>

#### Async::AttachAwait

```cpp
void AttachAwait([Data::PropertySet](#sym-13039646562789154425)& object, [Key32](#sym-974353891938499275) clock_id, [TRef <Task>](#sym-8974699438245378763) task, const [Function <void(bool, Object &)>](#sym-5470074934380427979)& callback);
void AttachAwait([GLX::Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) clock_id, [TRef <Task>](#sym-8974699438245378763) task, const [Function <void(bool, Object &)>](#sym-5470074934380427979)& callback);
```

Attaches a completion callback to a running task, bound to the lifetime of a host object.
Internally, a periodic clock is stored as a named property on the host object, which polls the task status and invokes the callback once the task completes. Because the clock is owned by the host object, it is released when the object is destroyed, ensuring the callback is never called after the host has been destructed.
Two overloads are provided: one for Data::PropertySet (general use), one for GLX::Object (UI code).
- object: The host object that owns the await state. The callback lifetime is tied to this object.
- clock_id: Id for the clock used for polling, attaching a new await with the same id replaces any existing one.
- task: The running task to observe.
- callback: Invoked on completion with 'ok' indicating success and 'result' payload object.

```cpp
// create and start a background task
auto task = New<Async::Worker>([](Async::Worker::Context& ctx)
{
// ... do work, call ctx.SetResult when done
});

// attach completion callback - lifetime tied to *this
// if *this is destroyed before the task completes, callback is never called
Async::AttachAwait(*this, "analyse", task, [this](bool ok, Object & result)
{
	if (ok)
	{
		auto data = DynamicCast<Allocation<Float32>>(result);
		m_display.Redraw();
	}
});
```

Note: for UI code, prefer the 'GLX::Object' overload which guarantees callbacks are delivered in a valid UI context. Do not use 'Async::CreatePeriodicClock' for completion monitoring in UI code - use 'AttachAwait' or the GLX clock equivalents instead.
<a id="sym-2674671305808173629"></a>

#### Async::CancelAwait

```cpp
void CancelAwait([Data::PropertySet](#sym-13039646562789154425)& object, [Key32](#sym-974353891938499275) clock_id);
```

<a id="sym-16961170023498406461"></a>

#### Async::CreateClock

```cpp
[TRef <Object>](#sym-8974699438245378763) CreateClock(const [Function <void()>](#sym-5470074934380427979)& callback);
```

<a id="sym-16352619813884942909"></a>

#### Async::CreatePeriodicClock

```cpp
[TRef <Object>](#sym-8974699438245378763) CreatePeriodicClock([Float32](#sym-1452730841175390923) interval, const [Function <void()>](#sym-5470074934380427979)& callback);
```

<a id="sym-3761071570620451947"></a>

#### Worker::Context

```cpp
bool Context::Cancelled();
```

```cpp
void Context::SetProgress([Float32](#sym-1452730841175390923) value_normalized);
```

```cpp
void Context::SetResult(bool result, [TRef <Object>](#sym-8974699438245378763) payload);
```

<a id="sym-8974771601801239101"></a>

#### Async::Task

Inherits from [System::Thread](#sym-15234142333668810788)
```cpp
void Task::Cancel();
```

```cpp
[Float32](#sym-1452730841175390923) Task::GetProgress();
```

```cpp
[Task::Status](#sym-15124628141771903332) Task::GetStatus();
```

```cpp
[TRef <Object>](#sym-8974699438245378763) Task::GetResult();
```

<a id="sym-15774081169288616509"></a>

#### Async::Worker

Inherits from [System::Thread](#sym-15234142333668810788)
---


##### [Reflex](#sym-14880872606059205893) > Data

<a id="sym-1018804717929684601"></a>

#### Data::kBinaryFormat

Binary serialization for PropertySet data.
Stores ArchiveObject properties only, using the Reflex binary container format.
Supported serialized property type: ArchiveObject.
```cpp
const kBinaryFormat kBinaryFormat;
```

<a id="sym-6310041514197180025"></a>

#### Data::kJsonFormat

While Reflex’s native formats (text-based PropertySheet and the binary PropertySet format) provide better type support and integration with the Reflex pipeline, JSON remains the natural choice when exchanging data with external systems - especially web services, scripting environments, and tools that expect standard JSON encoding.
Reflex::Data provides support for mapping JSON to/from PropertySet, allowing interchange while retaining the benefits of PropertySet’s typed accessors and structured hierarchy.

#### Decoding JSON into a PropertySet

Use DecodePropertySet with the Data::kJsonFormat format to parse a JSON blob into a standard PropertySet:
```cpp
Data::Archive json_blob = File::Open(path);
auto json = Data::DecodePropertySet(Data::kJSON, json_blob);
```

Once decoded, use the dynamic property system to access the properties
For readability/convienence and to minimise template bloat, Data implements helpers for common property types:
```cpp
auto f = Data::GetFloat32(json, "my_key");
auto s = Data::GetCString(json, "my_string");
auto sub = Data::GetPropertySet(json, "sub_node");
```

Nested structure, typed values, and optional presence checks behave exactly like any other PropertySet.

### Array Properties

Use the Data::Get...Array helpers to retrieve arrays, including Data::GetPropertySetArray for object arrays.
```cpp
auto items = Data::GetFloat32Array(json, "numbers");
for (auto & i : items)
{
}
```


### Root-level JSON arrays

PropertySet is a map, not an array.
To read a JSON document whose root is an array, use:
```cpp
auto items = Data::GetPropertySetArray(json, {});  //or kNullKey for constexpr hash of ""
for (auto & i : items)
{
}
```

Each entry in the returned array is a PropertySet representing one array element.

### JSON decoding options

Requires use of KeyMap to associate Key32 id's with their source string.
These are controlled via the optional Decode options structure passed to 'DecodePropertySet'.

#### Encoding PropertySet to JSON

Encoding follows the same Format interface as all other Data formats, but one key rule applies:

### JSON keys must be strings.

PropertySet keys are integer IDs, so each ID must be mapped to a string name.

### Registering keys

Before setting values on a JSON PropertySet for encoding, acquire a key map on the root:
```cpp
Data::PropertySet json;
auto keymap = Data::AcquireKeyMap(json);   // only needed on the root
```

Then register each string key once:
```cpp
auto my_key = Data::RegisterKey(keymap, "my_key");
Data::SetFloat32(json, my_key, 22.0f);
```

After registration, the same integer ID is used for subsequent sets - no repeated string hashing.

### Encoding to JSON

```cpp
auto blob = Data::EncodePropertySet(Data::kJSON, json);
File::Save(path, blob);
```

The resulting 'Data::Archive' contains a UTF-8 JSON document.

#### Supported types

- PropertySet
- PropertySetArray
- BoolProperty
- UInt32Property
- UInt64Property
- Int32Property
- Int64Property
- Float32Property
- Float64Property
- CStringProperty
- WStringProperty


#### Summary

- Use DecodePropertySet / EncodePropertySet with 'kJsonFormat' for full JSON <-> PropertySet interoperability.
- Use typed accessors (GetFloat32, GetCString, GetPropertySet, etc.) exactly as with native PropertySet formats.
- JSON keys must be string-mapped via AcquireKeyMap / RegisterKey.
- Root arrays are accessed using 'GetPropertySetArray(json, kNullKey)'.
- JSON decoding/encoding offers flexible type options for strings and numeric widths.

```cpp
const kJsonFormat kJsonFormat;
```

<a id="sym-8978219608157773433"></a>

#### Data::kLZ4

```cpp
const kLZ4 kLZ4;
```

<a id="sym-16767242442695040633"></a>

#### Data::kPropertySetFormat

```cpp
const kPropertySetFormat kPropertySetFormat;
```

<a id="sym-14488901298783575673"></a>

#### Data::kPropertySheetFormat

Human-readable Reflex property-sheet serializer.

#### Supported types

- PropertySet
- PropertySetArray
- BoolProperty
- UInt32Property
- UInt64Property
- Int32Property
- Int64Property
- Float32Property
- Float64Property
- CStringProperty
- WStringProperty
- Key32Property
- ArchiveObject

```cpp
const kPropertySheetFormat kPropertySheetFormat;
```

<a id="sym-13193545862458957433"></a>

#### Data::kReflexMarkupFormat

```cpp
const kReflexMarkupFormat kReflexMarkupFormat;
```

<a id="sym-1588972630179896953"></a>

#### Data::kReflexXmlFormat

Reflex XML support is for interoperability (eg SVG, simple config).
XML is decoded into a PropertySet tree where:
- Each XML element is a PropertySet
- Child elements live in a PropertySetArray stored at the root/empty key (kNullKey)
- The element tag name is stored in a string property "tag"
- Attributes (and any other element values) are stored as string properties on the element PropertySet


#### Decoding XML into a PropertySet

```cpp
Data::Archive blob = File::Open(path);
auto xml = Data::DecodePropertySet(Data::kReflexXmlFormat, blob);
```

The returned PropertySet is the root element.

#### Reading the root tag

```cpp
if (Data::GetXmlTag(*xml) == "svg")
{
}
```

XML documents have a single root element.

#### Iterating child elements

Child elements are stored as a PropertySetArray at kNullKey.
```cpp
for (auto & child : Data::GetXmlNodes(xml))
{
	auto tag = Data::GetXmlTag(*child);
}
```


#### Reading attributes / properties

XML attribute values decode as strings and are stored directly on the node PropertySet.
Use the dynamic property iterator (or your string helpers) to read them.
```cpp
auto keymap = Data::GetKeyMap(xml);
for (auto & child : Data::GetXmlNodes(xml))
{
	for (auto & [adr, prop] : child->Iterate<Data::CStringProperty>())
	{
		auto name  = Data::GetKey(keymap, adr.id);
		auto value = prop->value;
	}
}
```

Note: "tag" is also a string property on the node. Skip it if you only want attributes.

#### Building XML trees (for encoding)

To build an XML structure, create a root PropertySet and attach child nodes via the kNullKey array.

### Adding / accessing the child-node array

```cpp
Data::PropertySet root;
auto nodes = Data::AcquireXmlNodes(root);   // = AcquirePropertySetArray(root, kNullKey)
```


### Adding a child element

```cpp
auto rect = Data::AddXmlNode(*nodes, "rect");  // creates a PropertySet + sets ktag
Data::SetCString(rect, "x", "2");
Data::SetCString(rect, "y", "3");
Data::SetCString(rect, "width", "10");
Data::SetCString(rect, "height", "10");
```

To add grandchildren, call AcquireXmlNodes on the child and keep going.
```cpp
auto g = Data::AddXmlNode(*nodes, "g");
Data::SetCString(g, "transform", "translate(12 12)");
auto g_nodes = Data::AcquireXmlNodes(*g);
auto path = Data::AddXmlNode(*g_nodes, "path");
Data::SetCString(path, "d", "M0 0 L10 0");
```


#### Encoding to XML

```cpp
auto blob = Data::EncodePropertySet(Data::kReflexXmlFormat, root);
File::Save(path, blob);
```


#### Practical notes / constraints

- All XML values are strings (attributes and any stored element values). Convert manually if you need numeric types.
- Child elements are always in the kNullKey PropertySetArray (AcquireXmlNodes / GetXmlNodes).
- Tag name lives in the 'ktag' CString on each node (GetXmlTag).
- If your XML subset is “icons/SVG-like”, you can usually ignore namespaces, DTD, entities, etc.


#### Supported types

- PropertySetArray (xml elements)
- CStringProperty (xml attributes)


#### Summary

- Decode with DecodePropertySet(kReflexXmlFormat, ...)
- Root element is a PropertySet; tag via GetXmlTag.
- Children via GetXmlNodes(node) (stored at kNullKey).
- Build trees with AcquireXmlNodes + AddXmlNode.
- Attributes are string properties on the node PropertySet.

```cpp
const kReflexXmlFormat kReflexXmlFormat;
```

<a id="sym-11560583731558276729"></a>

#### Data::Archive

```cpp
using Archive = [Array <UInt8>](#sym-925399584115751627);
```

<a id="sym-3410220346311237241"></a>

#### Data::Archive::View

```cpp
using Archive::View = [ArrayView <UInt8>](#sym-17111432006044187339);
```

<a id="sym-6318097438325399161"></a>

#### Data::Float32Property

```cpp
using Float32Property = [ObjectOf <Float32>](#sym-15739513838455978699);
```

<a id="sym-5532383564373483129"></a>

#### Data::Float64Property

```cpp
using Float64Property = [ObjectOf <Float64>](#sym-15739513838455978699);
```

<a id="sym-4261681755723980409"></a>

#### Data::Int32Property

```cpp
using Int32Property = [ObjectOf <Int32>](#sym-15739513838455978699);
```

<a id="sym-3475967881772064377"></a>

#### Data::Int64Property

```cpp
using Int64Property = [ObjectOf <Int64>](#sym-15739513838455978699);
```

<a id="sym-13707062997161139833"></a>

#### Data::KeyMap

```cpp
using KeyMap = [ObjectOf < Map < Key <UInt32> , Array <char> > >](#sym-15739513838455978699);
```

<a id="sym-9719418972175130233"></a>

#### Data::UInt32Property

```cpp
using UInt32Property = [ObjectOf <UInt32>](#sym-15739513838455978699);
```

<a id="sym-8933705098223214201"></a>

#### Data::UInt64Property

```cpp
using UInt64Property = [ObjectOf <UInt64>](#sym-15739513838455978699);
```

<a id="sym-5361974431892692601"></a>

#### Data::AcquireKeyMap

```cpp
[TRef < ObjectOf < Map < Key <UInt32> , Array <char> > > >](#sym-8974699438245378763) AcquireKeyMap([PropertySet](#sym-13039646562789154425)& root);
```

<a id="sym-6835714230980763257"></a>

#### Data::AcquirePropertySet

```cpp
[TRef <PropertySet>](#sym-8974699438245378763) AcquirePropertySet([PropertySet](#sym-13039646562789154425)& propertyset, [Key32](#sym-974353891938499275) id);
[TRef <PropertySet>](#sym-8974699438245378763) AcquirePropertySet([PropertySet](#sym-13039646562789154425)& propertyset, const [ArrayView < Key <UInt32> >](#sym-17111432006044187339)& path);
```

<a id="sym-9716146481973485177"></a>

#### Data::AcquirePropertySetArray

```cpp
[TRef < ObjectArray <PropertySet> >](#sym-8974699438245378763) AcquirePropertySetArray([PropertySet](#sym-13039646562789154425)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-103905494798753401"></a>

#### Data::AddPropertySet

```cpp
[TRef <PropertySet>](#sym-8974699438245378763) AddPropertySet([ObjectArray <PropertySet>](#sym-16772990762499497593)& array);
```

<a id="sym-5360487183502335609"></a>

#### Data::Assimilate

```cpp
void Assimilate([PropertySet](#sym-13039646562789154425)& target, const [PropertySet](#sym-13039646562789154425)& source);
```

<a id="sym-14859215565183049337"></a>

#### Data::BytesToHex

```cpp
[CString](#sym-17531698610406914763) BytesToHex(const [Archive::View](#sym-3410220346311237241)& bytes);
```

<a id="sym-930420767788686969"></a>

#### Data::CRC32

```cpp
[UInt32](#sym-15243775351508400843) CRC32(const [Archive::View](#sym-3410220346311237241)& bytes, [UInt32](#sym-15243775351508400843) previous);
```

Computes a standard 32-bit CRC (Cyclic Redundancy Check) using the IEEE 802.3 polynomial. Intended primarily for error detection or compatibility with external systems that use CRC32.
- bytes: Binary data to process.
- previous: Optional seed value. Can be used to continue hashing from a previous result.
- return: 32-bit CRC value.

Note, if 'previous' is non-zero, the function will continue from that value, allowing incremental CRC accumulation over multiple buffers. This is useful for streaming or chunked data validation.
```cpp
UInt32 crc = CRC32(chunk1);
crc = CRC32(chunk2, crc);
crc = CRC32(chunk3, crc);
```

<a id="sym-13248356113376665209"></a>

#### Data::Compress

```cpp
[Archive](#sym-11560583731558276729) Compress(const [CompressionAlgorithm](#sym-7545490347245231737)& algorithm, const [Archive::View](#sym-3410220346311237241)& bytes);
```

<a id="sym-12382521355389689465"></a>

#### Data::CopyPropertySet

```cpp
[PropertySet](#sym-13039646562789154425) CopyPropertySet(const [Format](#sym-12916641001534185081)& format, const [PropertySet](#sym-13039646562789154425)& propertyset);
```

<a id="sym-4196140279909113465"></a>

#### Data::DecodePropertySet

```cpp
[PropertySet](#sym-13039646562789154425) DecodePropertySet(const [Format](#sym-12916641001534185081)& format, const [Archive::View](#sym-3410220346311237241)& bytes, [UInt32](#sym-15243775351508400843) options);
```

<a id="sym-17015155771860639353"></a>

#### Data::DecodeUCS2

```cpp
void DecodeUCS2([WString](#sym-17787487981880441547)& output, const [Archive::View](#sym-3410220346311237241)& bytes);
[WString](#sym-17787487981880441547) DecodeUCS2(const [Archive::View](#sym-3410220346311237241)& bytes);
```

<a id="sym-17015233467819023993"></a>

#### Data::DecodeUTF8

```cpp
void DecodeUTF8([WString](#sym-17787487981880441547)& output, const [Archive::View](#sym-3410220346311237241)& bytes);
[WString](#sym-17787487981880441547) DecodeUTF8(const [Archive::View](#sym-3410220346311237241)& bytes);
```

<a id="sym-4808060509841579641"></a>

#### Data::DecodeUrlSegment

```cpp
[CString](#sym-17531698610406914763) DecodeUrlSegment(const [CString::View](#sym-2022350079943473867)& view);
```

<a id="sym-141847145697304185"></a>

#### Data::Decompress

```cpp
[Archive](#sym-11560583731558276729) Decompress(const [DecompressionAlgorithm](#sym-487480056112799353)& algorithm, const [Archive::View](#sym-3410220346311237241)& bytes);
```

<a id="sym-13657719981913202297"></a>

#### Data::Deserialize

```cpp
void Deserialize([Archive::View](#sym-3410220346311237241)& stream, VARGS...& ...);
TYPE Deserialize([Archive::View](#sym-3410220346311237241)& stream);
[Tuple <VARGS...>](#sym-1022631093872065227) Deserialize([Archive::View](#sym-3410220346311237241)& stream);
```

Restores one or more values from a binary [code Archive::View], in the same order they were previously stored. Two forms are provided for flexibility.
- stream: Source archive view to decode from.
- VARGS...: The types to restore, either as references or template parameters.
- return: Either void (for reference-based restore), or a [code Tuple] containing the restored values.


#### Restore by references

```cpp
void Deserialize(Archive::View & stream, VARGS & ...)
```

Restores values in-place by writing into references.
```cpp
Int32 a;
CString b;
Restore(stream, a, b);
```


#### Restore by return

```cpp
template <class ...VARGS> Tuple<VARGS...> Deserialize(Archive::View & stream)
```

Restores values and returns them as a [code Tuple]. If only one type is requested, the tuple decays to that type automatically.
```cpp
auto [id, name] = Deserialize<Int32,CString>(in);
CString str = Deserialize<CString>(in); // returns single value directly
```

The input [code stream] is advanced as each value is decoded. The number and order of types must match what was passed to [code Store].
<a id="sym-4064969010866871929"></a>

#### Data::DeserializePropertySet

```cpp
void DeserializePropertySet([Archive::View](#sym-3410220346311237241)& stream, const [SerializableFormat](#sym-5671468602716380793)& format, [PropertySet](#sym-13039646562789154425)& out);
```

<a id="sym-16231191942463020665"></a>

#### Data::DeserializeUCS2

```cpp
void DeserializeUCS2([Archive::View](#sym-3410220346311237241)& stream, [WString](#sym-17787487981880441547)& string_out);
[WString](#sym-17787487981880441547) DeserializeUCS2([Archive::View](#sym-3410220346311237241)& stream);
```

<a id="sym-16231269638421405305"></a>

#### Data::DeserializeUTF8

```cpp
void DeserializeUTF8([Archive::View](#sym-3410220346311237241)& stream, [WString](#sym-17787487981880441547)& string_out);
[WString](#sym-17787487981880441547) DeserializeUTF8([Archive::View](#sym-3410220346311237241)& stream);
```

<a id="sym-4247992054274121337"></a>

#### Data::EncodePropertySet

```cpp
[Archive](#sym-11560583731558276729) EncodePropertySet(const [Format](#sym-12916641001534185081)& format, const [PropertySet](#sym-13039646562789154425)& propertyset);
```

<a id="sym-616012591737659001"></a>

#### Data::EncodeUCS2

```cpp
void EncodeUCS2([Archive](#sym-11560583731558276729)& output, const [WString::View](#sym-5727895666471172811)& string);
[Archive](#sym-11560583731558276729) EncodeUCS2(const [WString::View](#sym-5727895666471172811)& string);
```

<a id="sym-616090287696043641"></a>

#### Data::EncodeUTF8

```cpp
void EncodeUTF8([Archive](#sym-11560583731558276729)& output, const [WString::View](#sym-5727895666471172811)& string);
[Archive](#sym-11560583731558276729) EncodeUTF8(const [WString::View](#sym-5727895666471172811)& string);
```

<a id="sym-896686063126371961"></a>

#### Data::EncodeUrlSegment

```cpp
[CString](#sym-17531698610406914763) EncodeUrlSegment(const [CString::View](#sym-2022350079943473867)& view);
[CString](#sym-17531698610406914763) EncodeUrlSegment(const [WString::View](#sym-5727895666471172811)& view);
```

<a id="sym-14722083040447749753"></a>

#### Data::FNV1a32

```cpp
[UInt32](#sym-15243775351508400843) FNV1a32(const [Archive::View](#sym-3410220346311237241)& bytes, [UInt32](#sym-15243775351508400843) previous);
```

<a id="sym-14722083474239446649"></a>

#### Data::FNV1a64

```cpp
[UInt64](#sym-15243775785300097739) FNV1a64(const [Archive::View](#sym-3410220346311237241)& bytes, [UInt64](#sym-15243775785300097739) previous);
```

Computes a fast non-cryptographic hash using the FNV-1a algorithm. Suitable for hashing structured binary data (strings, file names, serialized values).
- bytes: Binary data to hash
- seed: Optional seed value. Can be used to continue hashing from a previous result
- return: 64-bit hash value

```cpp
auto file = Make<System::FileHandle>(path_to_file);
auto h64 = FNV1a64({});
while (auto chunk = File::ReadBytes(file, kMaxUInt16))
{

}
```

<a id="sym-5843614929209319033"></a>

#### Data::GetBool

```cpp
bool GetBool(const [Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, bool fallback_opt);
```

<a id="sym-7490243057635352185"></a>

#### Data::GetFloat32

```cpp
[Float32](#sym-1452730841175390923) GetFloat32(const [Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [Float32](#sym-1452730841175390923) fallback_opt);
```

<a id="sym-7490243491427049081"></a>

#### Data::GetFloat64

```cpp
[Float64](#sym-1452731274967087819) GetFloat64(const [Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [Float64](#sym-1452731274967087819) fallback_opt);
```

<a id="sym-8407367511912275577"></a>

#### Data::GetInt32

```cpp
[Int32](#sym-965532656177740491) GetInt32(const [Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [Int32](#sym-965532656177740491) fallback_opt);
```

<a id="sym-8407367945703972473"></a>

#### Data::GetInt64

```cpp
[Int64](#sym-965533089969437387) GetInt64(const [Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [Int64](#sym-965533089969437387) fallback_opt);
```

<a id="sym-13033941574909746809"></a>

#### Data::GetKey

```cpp
[CString::View](#sym-2022350079943473867) GetKey(const [KeyMap](#sym-13707062997161139833)& keymap, [Key32](#sym-974353891938499275) key);
```

<a id="sym-8416188747673034361"></a>

#### Data::GetKey32

```cpp
[Key32](#sym-974353891938499275) GetKey32(const [Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [Key32](#sym-974353891938499275) fallback_opt);
```

<a id="sym-1033196285755186809"></a>

#### Data::GetKeyMap

```cpp
[ConstTRef < ObjectOf < Map < Key <UInt32> , Array <char> > > >](#sym-532369501975706315) GetKeyMap(const [PropertySet](#sym-13039646562789154425)& root);
```

<a id="sym-14089334233469345401"></a>

#### Data::GetPropertySet

```cpp
[ConstTRef <PropertySet>](#sym-532369501975706315) GetPropertySet(const [Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id);
[ConstTRef <PropertySet>](#sym-532369501975706315) GetPropertySet(const [PropertySet](#sym-13039646562789154425)& propertyset, const [ArrayView < Key <UInt32> >](#sym-17111432006044187339)& path);
```

<a id="sym-5537952500573855353"></a>

#### Data::GetPropertySetArray

```cpp
[ArrayView < ConstReference <PropertySet> >](#sym-17111432006044187339) GetPropertySetArray(const [PropertySet](#sym-13039646562789154425)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-2569908637639171705"></a>

#### Data::GetUInt32

```cpp
[UInt32](#sym-15243775351508400843) GetUInt32(const [Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [UInt32](#sym-15243775351508400843) fallback_opt);
```

<a id="sym-2569909071430868601"></a>

#### Data::GetUInt64

```cpp
[UInt64](#sym-15243775785300097739) GetUInt64(const [Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [UInt64](#sym-15243775785300097739) fallback_opt);
```

<a id="sym-8462759705128787577"></a>

#### Data::GetUInt8

```cpp
[UInt8](#sym-1020924849394252491) GetUInt8(const [Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [UInt8](#sym-1020924849394252491) fallback_opt);
```

<a id="sym-6238121638285598329"></a>

#### Data::HexToBytes

```cpp
[Archive](#sym-11560583731558276729) HexToBytes(const [CString::View](#sym-2022350079943473867)& hex);
```

<a id="sym-627316825401646713"></a>

#### Data::IsHttps

```cpp
bool IsHttps(const [CString::View](#sym-2022350079943473867)& url);
```

<a id="sym-1518101572397096569"></a>

#### Data::MakeUrl

```cpp
[CString](#sym-17531698610406914763) MakeUrl(const [Url](#sym-830962064086326905)& url);
```

<a id="sym-984515721968017017"></a>

#### Data::Merge

```cpp
[PropertySet](#sym-13039646562789154425) Merge(const [PropertySet](#sym-13039646562789154425)& a, const [PropertySet](#sym-13039646562789154425)& b);
```

<a id="sym-8974151939121012345"></a>

#### Data::Pack

```cpp
[Archive::View](#sym-3410220346311237241) Pack(const TYPE& value);
```

Converts a value into a binary view without transformation.
Pack operates only on *raw-packable types* - types whose binary representation is stable, contiguous, and platform-independent.
- value: The value to pack. TYPE must be a raw-packable type.
- return: An Archive::View referencing the binary representation of the value.

If TYPE is not raw-packable, compilation will fail with an error such as “Non raw-packable type”.
Pack performs no encoding, metadata emission, or length prefixing. The size of the returned view is determined entirely by the value’s binary representation.
```cpp
UInt32 u32 = 42;
auto view = Data::Pack(u32);      //OK
```

```cpp
CString value;                    //typedef of Array<char>
auto view = Data::Pack(value);    //OK
```

```cpp
WString value;
auto view = Data::Pack(value);    //ERROR wchar_t size is platform dependent, use Data::EncodeUTF8 instead
```

This struct is raw-packable:
```cpp
struct Foo
{
	UInt32 a;
	CString b;
};

Foo foo;
auto view = Data::Pack(foo);	//OK
```

Whereas this struct is not:
```cpp
struct Foo
{
	UInt32 a;
	WString b;
};

Foo foo;
auto view = Data::Pack(foo);	//ERROR
```

<a id="sym-8143884342183124601"></a>

#### Data::ReadLine

```cpp
bool ReadLine([Archive::View](#sym-3410220346311237241)& stream, [WString](#sym-17787487981880441547)& line_out);
bool ReadLine([Archive::View](#sym-3410220346311237241)& stream, [CString](#sym-17531698610406914763)& line_out);
```

Reads a single line of text from a memory stream and decodes it as either raw ASCII (CString overload) or UTF-8 (WString overload).
- stream: An Archive::View containing text data.
- line_out: Output buffer receiving the decoded line. CString reads raw ASCII bytes; WString decodes from UTF-8.
- return: true if a line was read (including empty lines); false only when no more lines are available (end-of-file).

```cpp
//read entire ASCII text file
Data::Archive archive = File::Open(path);
Data::Archive::View stream = archive;	//use ToView for shorthand
CString buffer;
while (Data::ReadLine(stream, buffer))
{
	//process line
}
```

<a id="sym-12302724445032734329"></a>

#### Data::RegisterKey

```cpp
[Key32](#sym-974353891938499275) RegisterKey([KeyMap](#sym-13707062997161139833)& keymap, const [CString::View](#sym-2022350079943473867)& string);
```

<a id="sym-17882451587527401081"></a>

#### Data::ResetPropertySet

```cpp
void ResetPropertySet(const [Format](#sym-12916641001534185081)& format, [PropertySet](#sym-13039646562789154425)& propertyset);
```

<a id="sym-8974492985294118521"></a>

#### Data::SHA1

```cpp
[Archive](#sym-11560583731558276729) SHA1(const [Archive::View](#sym-3410220346311237241)& bytes);
```

Computes a 160-bit SHA-1 hash of the input data. Suitable for content identification or integrity checks where a stronger hash than CRC32 is required.
- bytes: Binary data to hash.
- return: 160-bit SHA-1 hash result, typically used for content-based identification.

Note, SHA-1 is not considered secure for cryptographic purposes but remains useful for non-security-critical fingerprinting or deduplication.
```cpp
Archive hash = SHA1(fileData);
```

<a id="sym-14895257324413312633"></a>

#### Data::SHA256

```cpp
[Archive](#sym-11560583731558276729) SHA256(const [Archive::View](#sym-3410220346311237241)& bytes);
```

Computes a 256-bit SHA-2 (SHA-256) hash of the input data. Provides strong collision resistance for security-sensitive applications such as signing, verification, or integrity validation.
- bytes: Binary data to hash.
- return: 256-bit SHA-256 hash result.

Note, SHA-256 is suitable for cryptographic use cases, including secure hashing of content, signatures, and tokens.
```cpp
Archive hash = SHA256(fileData);
```

<a id="sym-3450658146302877305"></a>

#### Data::Serialize

```cpp
void Serialize([Archive](#sym-11560583731558276729)& stream, const VARGS...& ...);
```

Serializes one or more values into a binary [code Archive]. Supports any combination of raw, structured, or user-defined types with appropriate [code Encode] overloads.
- stream: Destination archive to append serialized data into.
- VARGS...: One or more values to serialize. Can be of any supported type.
- return: (void) The archive is modified in-place.

This function is the core of Reflex's structured serialization system. It can encode basic types, arrays, containers, strings, and custom structures, in order.
```cpp
Archive out;
Serialize(out, Int32(42), CString("example"), my_object);
```

The data can later be recovered using [code Restore] with the same types and order.
```cpp
Archive::View in = out;
Int32 a;
CString b;
MyType c;
Deserialize(in, a, b, c);
```

<a id="sym-11122979301999304313"></a>

#### Data::SerializePropertySet

```cpp
void SerializePropertySet([Archive](#sym-11560583731558276729)& stream, const [SerializableFormat](#sym-5671468602716380793)& format, const [PropertySet](#sym-13039646562789154425)& in);
```

<a id="sym-713211318008602233"></a>

#### Data::SerializeUCS2

```cpp
void SerializeUCS2([Archive](#sym-11560583731558276729)& stream, const [WString::View](#sym-5727895666471172811)& string);
```

<a id="sym-713289013966986873"></a>

#### Data::SerializeUTF8

```cpp
void SerializeUTF8([Archive](#sym-11560583731558276729)& stream, const [WString::View](#sym-5727895666471172811)& string);
```

<a id="sym-8029977604720221817"></a>

#### Data::SetBinary

```cpp
void SetBinary([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, const [Archive::View](#sym-3410220346311237241)& value);
```

<a id="sym-17065134996319166073"></a>

#### Data::SetBool

```cpp
void SetBool([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, bool value);
```

<a id="sym-8616923115222523513"></a>

#### Data::SetCString

```cpp
void SetCString([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, const [CString::View](#sym-2022350079943473867)& value);
```

<a id="sym-10984699419700551289"></a>

#### Data::SetFloat32

```cpp
void SetFloat32([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [Float32](#sym-1452730841175390923) value);
```

<a id="sym-10984699853492248185"></a>

#### Data::SetFloat64

```cpp
void SetFloat64([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [Float64](#sym-1452731274967087819) value);
```

<a id="sym-9782648252346195577"></a>

#### Data::SetInt32

```cpp
void SetInt32([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [Int32](#sym-965532656177740491) value);
```

<a id="sym-9782648686137892473"></a>

#### Data::SetInt64

```cpp
void SetInt64([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [Int64](#sym-965533089969437387) value);
```

<a id="sym-9791469488106954361"></a>

#### Data::SetKey32

```cpp
void SetKey32([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [Key32](#sym-974353891938499275) value);
```

<a id="sym-9982811737137667705"></a>

#### Data::SetPropertySet

```cpp
void SetPropertySet([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [TRef <PropertySet>](#sym-8974699438245378763) value);
```

<a id="sym-11060684924539428473"></a>

#### Data::SetUInt32

```cpp
void SetUInt32([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [UInt32](#sym-15243775351508400843) value);
```

<a id="sym-11060685358331125369"></a>

#### Data::SetUInt64

```cpp
void SetUInt64([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [UInt64](#sym-15243775785300097739) value);
```

<a id="sym-9838040445562707577"></a>

#### Data::SetUInt8

```cpp
void SetUInt8([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, [UInt8](#sym-1020924849394252491) value);
```

<a id="sym-8872712486696050297"></a>

#### Data::SetWString

```cpp
void SetWString([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id, const [WString::View](#sym-5727895666471172811)& value);
```

<a id="sym-14276455779483573881"></a>

#### Data::SplitUrl

```cpp
[Url](#sym-830962064086326905) SplitUrl(const [CString::View](#sym-2022350079943473867)& url);
```

<a id="sym-3947668733844053625"></a>

#### Data::SplitUrlResource

```cpp
[Pair < ArrayView <char> , ArrayView <char> >](#sym-8974152822052584139) SplitUrlResource(const [CString::View](#sym-2022350079943473867)& url);
```

<a id="sym-15432461427091234425"></a>

#### Data::Unpack

```cpp
void Unpack(const [Archive::View](#sym-3410220346311237241)& bytes, TYPE& output);
TYPE Unpack(const [Archive::View](#sym-3410220346311237241)& bytes);
```

Restores a value from a binary view, previously produced by Pack.
Unpack operates only on *raw-packable types* and reconstructs the value by reading its binary representation.
- ref: An Archive::View referencing the binary data to unpack.
- value: Destination for the unpacked value (overload 1).
- return: The unpacked value (overload 2).

TYPE must match the type used when packing. If TYPE is not raw-packable, compilation will fail with an error similar to "Non raw-packable type".
Unpack performs no decoding, validation, or metadata processing. It simply interprets the referenced bytes as TYPE.
Unpack assumes the binary layout in ref exactly matches TYPE.  Mismatched types or incompatible layouts result in undefined behavior.
Use Unpack only with data produced by Pack or when the binary layout is known and stable.
```cpp
UInt32 original = 42;
auto view = Data::Pack(original);

UInt32 restored;
Data::Unpack(view, restored);	//OK
```

```cpp
UInt32 restored = Data::Unpack<UInt32>(view);	//OK
```

```cpp
struct Foo
{
	UInt32 a;
	CString b;
};

Foo foo;
auto view = Data::Pack(foo);

Foo restored = Data::Unpack<Foo>(view);
```

```cpp
auto restored = Data::Unpack<WString>(view);	//FAIL WString size is platform dependent
```

<a id="sym-18203019975041539705"></a>

#### Data::UnsetBinary

```cpp
void UnsetBinary([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-12399268768611622521"></a>

#### Data::UnsetBool

```cpp
void UnsetBool([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-12285928009054084729"></a>

#### Data::UnsetCString

```cpp
void UnsetCString([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-14653704313532112505"></a>

#### Data::UnsetFloat32

```cpp
void UnsetFloat32([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-14653704747323809401"></a>

#### Data::UnsetFloat64

```cpp
void UnsetFloat64([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-3383015327673671289"></a>

#### Data::UnsetInt32

```cpp
void UnsetInt32([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-3383015761465368185"></a>

#### Data::UnsetInt64

```cpp
void UnsetInt64([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-3391836563434430073"></a>

#### Data::UnsetKey32

```cpp
void UnsetKey32([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-15730379041849532025"></a>

#### Data::UnsetPropertySet

```cpp
void UnsetPropertySet([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-9256090634860682873"></a>

#### Data::UnsetPropertySetArray

```cpp
void UnsetPropertySetArray([PropertySet](#sym-13039646562789154425)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-2786983221151194745"></a>

#### Data::UnsetUInt32

```cpp
void UnsetUInt32([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-2786983654942891641"></a>

#### Data::UnsetUInt64

```cpp
void UnsetUInt64([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-3438407520890183289"></a>

#### Data::UnsetUInt8

```cpp
void UnsetUInt8([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-12541717380527611513"></a>

#### Data::UnsetWString

```cpp
void UnsetWString([Object](#sym-14361920786414140107)& propertyset, [Key32](#sym-974353891938499275) id);
```

<a id="sym-7367949705037274745"></a>

#### Data::WriteLine

```cpp
void WriteLine([Archive](#sym-11560583731558276729)& stream, const [WString::View](#sym-5727895666471172811)& line);
void WriteLine([Archive](#sym-11560583731558276729)& stream, const [CString::View](#sym-2022350079943473867)& line);
```

Writes a single line of text to a Data::Archive (byte array), encoding the input as either raw ASCII (CString::View overload) or UTF-8 (WString::View overload).
- stream: Byte array to append the encoded line.
- line: The line to write. CString::View is written as raw ASCII bytes; WString::View is encoded to UTF-8.

A line terminator is automatically appended. The input string should not include a newline.
```cpp
Data::Archive stream;

//write plain ASCII
Data::WriteLine(stream, "first line");
Data::WriteLine(stream, "second line");

//write multibyte UTF-8
Data::WriteLine(stream, L"UTF-8 text");
Data::WriteLine(stream, L"日本語");
```

<a id="sym-599668559877486605"></a>

#### SerializableFormat::DeserializeError

<a id="sym-8760467420767618014"></a>

#### template PropertySet::PropertyIterator <TYPE>

<a id="sym-830962064086326905"></a>

#### Data::Url

```cpp
[CString](#sym-17531698610406914763) Url::domain;
```

```cpp
[CString](#sym-17531698610406914763) Url::fragment;
```

```cpp
[UInt16](#sym-15243775085220428491) Url::port;
```

```cpp
[CString](#sym-17531698610406914763) Url::resource;
```

```cpp
[CString](#sym-17531698610406914763) Url::scheme;
```

```cpp
[Pair < Array <char> , Array <char> >](#sym-8974152822052584139) Url::user;
```

<a id="sym-7146817876374118009"></a>

#### Data::ArchiveObject

Inherits from [Object](#sym-14361920786414140107)
<a id="sym-7545490347245231737"></a>

#### Data::CompressionAlgorithm

Inherits from [DecompressionAlgorithm](#sym-487480056112799353)
```cpp
[UInt32](#sym-15243775351508400843) CompressionAlgorithm::GetMaxCompressedSize([UInt32](#sym-15243775351508400843) size);
```

```cpp
[UInt32](#sym-15243775351508400843) CompressionAlgorithm::Compress(const [Archive::View](#sym-3410220346311237241)& input, [UInt8](#sym-1020924849394252491)* output);
```

<a id="sym-487480056112799353"></a>

#### Data::DecompressionAlgorithm

Inherits from [Object](#sym-14361920786414140107)
```cpp
bool DecompressionAlgorithm::Decompress(const [Archive::View](#sym-3410220346311237241)& input, [Archive](#sym-11560583731558276729)& output);
```

<a id="sym-12916641001534185081"></a>

#### Data::Format

Inherits from [Object](#sym-14361920786414140107)
Data::Format defines the interface for encoding and decoding structured data.  A given Format implements how to serialize a PropertySet into bytes and how to reconstruct it back into a PropertySet.
Reflex provides several global, constant Format instances covering the common persistence use cases.

#### Common Formats


### kPropertySetFormat

- Recommended binary format
- Fast, compact, and supports the widest range of Reflex property objects: PropertySet, BoolProperty, UInt32Property, UInt64Property, Int32Property, Int64Property, Float32Property, Float64Property, CStringProperty, WStringProperty, Key32Property, ArchiveObject, and KeyMap.


### kPropertySheetFormat

- Human-readable text format
- Similar to JSON but with additional type info support.
- Supports PropertySet, PropertySetArray, BoolProperty, UInt32Property, UInt64Property, Int32Property, Int64Property, Float32Property, Float64Property, CStringProperty, WStringProperty, Key32Property, ArchiveObject, and KeyMap.


### kJsonFormat

- Standard JSON
- Ideal for interoperability with web APIs and external tooling.
- Supports PropertySet, PropertySetArray, BoolProperty, UInt32Property, UInt64Property, Int32Property, Int64Property, Float32Property, Float64Property, CStringProperty, WStringProperty, and KeyMap. Unsupported values are emitted as null.


### kReflexXmlFormat

- Supports a sub-set of XML
- Only attributes and nested elements (ignores text content between tags).
- Supports PropertySetArray, CStringProperty, BoolProperty, Int32Property, Float32Property, and KeyMap. Nested node payloads are represented as PropertySet children.


#### Typical Usage


### Encoding

```cpp
Data::PropertySet data;

Data::SetInt32(data, "version", 1);

auto child = Data::AcquirePropertySet(data, "child");
Data::SetFloat32Array(child, "values", {1.0f, 2.0f});

auto blob = Data::EncodePropertySet(Data::kPropertySetFormat, data);

File::Save(path, blob);
```


### Decoding

```cpp
auto bytes = File::Open(path);

Data::PropertySet root = Data::DecodePropertySet(Data::kPropertySetFormat, bytes);
if (root)
{
	auto v = Data::GetInt32(root, "version");
	auto sub = Data::GetPropertySet(root, "child");
	auto arr = Data::GetFloat32Array(sub, "values");
}
```


#### Choosing a Format

- Use kPropertySetFormat for application data, presets, configs, or anything Reflex-internal.
- Use kPropertySheetFormat when you need editable text files or want structured types without JSON’s restrictions.
- Use kJsonFormat for web communication and external file formats.

```cpp
void Format::Reset([PropertySet](#sym-13039646562789154425)& propertyset);
```

```cpp
bool Format::Encode([Archive](#sym-11560583731558276729)& out, const [PropertySet](#sym-13039646562789154425)& data);
```

```cpp
bool Format::Decode([PropertySet](#sym-13039646562789154425)& out, const [Archive::View](#sym-3410220346311237241)& data, [UInt32](#sym-15243775351508400843) options);
```

<a id="sym-16772990762499497593"></a>

#### template Data::ObjectArray <TYPE>

Inherits from [Object](#sym-14361920786414140107)
<a id="sym-13039646562789154425"></a>

#### Data::PropertySet

Inherits from [Object](#sym-14361920786414140107)
Data::PropertySet is Reflex’s generic container for structured, tree-based data.
PropertySet allows arbitrary data to be attached to objects at runtime, without modifying class definitions or introducing ad-hoc subclasses.
Instead of encoding every variation in a type hierarchy, behavior and state can be composed dynamically by attaching properties as needed.
```cpp
object.SetProperty("hover_time", 0.0f);
object.SetProperty("user_data", some_ref);
```

This bridges the performance and safety of C++ with the flexibility typically associated with dynamic languages.  It enables rapid iteration, late-bound features, and data-driven behavior while remaining fully type-aware and debuggable.

#### Generic property system

PropertySet implements the root Reflex::Object property interface (OnSetProperty / OnUnsetProperty / OnQueryProperty).
This provides a uniform, generic property mechanism across the entire framework.
Properties are identified by id AND type, rather than id alone.
This complements C++'s strong typing model by allowing multiple, type-safe views of the same conceptual property without collapsing everything into a single loosely-typed value.
```cpp
ps.SetProperty("my_id", 1);
ps.SetProperty("my_id", "string");
```

The example above creates two distinct properties:
- An Int32 property with id "my_id".
- A CString property with id "my_id".

Under the hood, this (id + type) pair is represented by [Reflex::Address](#sym-9214008371664091851).

#### Structured and hierarchical data

PropertySet supports hierarchical data by allowing nested PropertySet instances.
This makes it suitable for representing structured trees of runtime state, configuration, or metadata when needed.
Unlike rigid schemas, PropertySet allows structure to evolve organically as systems interact.

#### Persistence and serialization

PropertySet is serializable, via the Format system.
This makes it suitable for storing presets, state snapshots, configuration files, and interchange data.
For persistent data, values should be written using the Data::SetXXX family of functions, which define the set of interoperable, format-safe property types. Using the Reflex::Data:: suite of property accessors also avoids template bloat occuring from use of the Reflex:: templated ones.
```cpp
Data::SetFloat32(ps, "gain", 0.75f);
Data::SetCString(ps, "name", "Preset A");
```

PropertySet can be serialized using any Data::Format implementation:
```cpp
auto blob = Data::EncodePropertySet(Data::kPropertySetFormat, ps);
//or
auto json = Data::DecodePropertySet(Data::kJsonFormat, ps);
```

Note that when writing text-based formats (JSON, Reflex PropertySheets) you need to register the key-strings, via Data::AcquireKeyMap and Data::RegisterKey.

#### Summary

- PropertySet enables dynamic, runtime data attachment without subclassing.
- It is a core mechanism for flexible, data-driven behavior in Reflex.
- Properties are identified by (id, type), allowing rich runtime composition.
- Serialization is supported via pluggable formats.

Use Iterate<TYPE>() to iterate over all properties of TYPE
Each item will have a [key,value] where key is Reflex::Address (comprising of the property id and type_id) and value a Reference <TYPE>
```cpp
for (auto & [adr, ref] : test.Iterate<Data::CStringProperty>())
{
	output.Log(id.value, ref->value);
}
```

```cpp
bool PropertySet::Empty();
```

```cpp
bool PropertySet::operator bool();
```

```cpp
[PropertySet::PropertyIterator <TYPE>](#sym-8760467420767618014) PropertySet::Iterate();
```

<a id="sym-5671468602716380793"></a>

#### Data::SerializableFormat

Inherits from [Format](#sym-12916641001534185081)
```cpp
void SerializableFormat::Serialize([Archive](#sym-11560583731558276729)& stream, const [PropertySet](#sym-13039646562789154425)& propertyset);
```

```cpp
[SerializableFormat::DeserializeError](#sym-599668559877486605) SerializableFormat::Deserialize([Archive::View](#sym-3410220346311237241)& stream, [PropertySet](#sym-13039646562789154425)& propertyset);
```

---


##### [Reflex](#sym-14880872606059205893) > File

<a id="sym-15702586372901705791"></a>

#### File::kPathDelimiter

The standard path separator used by Reflex. This is an alias of System::kPathDelimiter and represents the canonical delimiter for all paths handled within Reflex APIs.
Reflex paths must always use this delimiter, regardless of the underlying OS. The System layer will translate paths when interacting with native APIs, ensuring portability across platforms. Any path provided by a Reflex::System API is guaranteed to use kPathDelimiter.
Although the current value is a forward slash ('/'), this should not be relied upon for future compatibility. All path construction should reference File::kPathDelimiter explicitly.

#### Trailing delimiter rules

Folder paths in Reflex typically include the trailing delimiter.
For example, when calling File::List(path), folder entries in the returned list will include the trailing separator.

#### Constructing a valid file path

Use Join(...) or manually insert File::kPathDelimiter when building paths:
```cpp
auto user_docs_path = File::GetSystemPath(File::kPathUserDocuments);   //includes trailing stroke
auto path = Join(user_docs_path, L"sub_folder", File::kPathDelimiter, L"sub_sub_folder", File::kPathDelimiter);
```

This ensures consistent behaviour across all platforms, correct interoperation with the VirtualFileSystem, and predictable results when listing or resolving paths.
```cpp
const kPathDelimiter kPathDelimiter;
```

<a id="sym-6450198791125604415"></a>

#### File::CheckExtension

```cpp
bool CheckExtension(const [WString::View](#sym-5727895666471172811)& path, const [WString::View](#sym-5727895666471172811)& extension);
```

<a id="sym-8972212795746687039"></a>

#### File::Copy

```cpp
bool Copy(const [WString](#sym-17787487981880441547)& from, const [WString](#sym-17787487981880441547)& to);
bool Copy([System::FileHandle](#sym-15071417447436378148)& from, [System::FileHandle](#sym-15071417447436378148)& to, [UInt32](#sym-15243775351508400843) chunksize_opt);
```

<a id="sym-2858318139697010751"></a>

#### File::CorrectExtension

```cpp
[WString](#sym-17787487981880441547) CorrectExtension(const [WString::View](#sym-5727895666471172811)& path, const [WString::View](#sym-5727895666471172811)& extension);
```

<a id="sym-16588900385359534143"></a>

#### File::CorrectStrokes

```cpp
[WString](#sym-17787487981880441547) CorrectStrokes(const [WString::View](#sym-5727895666471172811)& path);
```

<a id="sym-5910597418091681855"></a>

#### File::CorrectTrailingStroke

```cpp
[WString](#sym-17787487981880441547) CorrectTrailingStroke(const [WString::View](#sym-5727895666471172811)& path);
```

<a id="sym-15820803427873724479"></a>

#### File::CreateMemoryReader

```cpp
[TRef <System::FileHandle>](#sym-8974699438245378763) CreateMemoryReader([ConstTRef <Data::ArchiveObject>](#sym-532369501975706315) data);
```

<a id="sym-16728754609395472447"></a>

#### File::CreateMemoryWriter

```cpp
[TRef <System::FileHandle>](#sym-8974699438245378763) CreateMemoryWriter([TRef <Data::ArchiveObject>](#sym-8974699438245378763) data);
```

<a id="sym-12528574737652258879"></a>

#### File::Delete

```cpp
bool Delete(const [WString](#sym-17787487981880441547)& path);
```

Deletes a file or directory. To delete a directory, the directory must be empty.
This is an alias of System::Delete.
<a id="sym-4095212319434479679"></a>

#### File::DeleteDirectoryContent

```cpp
void DeleteDirectoryContent(const [WString](#sym-17787487981880441547)& path);
```

Deletes all content of a directory, recursively, but does not the directory itself.
<a id="sym-6772400799515547711"></a>

#### File::DeletePath

```cpp
void DeletePath(const [WString](#sym-17787487981880441547)& path);
```

Deletes a directory, including all content.
<a id="sym-12793038813487306815"></a>

#### File::Exists

```cpp
bool Exists(const [WString](#sym-17787487981880441547)& path);
```

Returns true is path is a file or directory/folder.
This is an alias of System::Exists.
<a id="sym-10722639413154814015"></a>

#### File::GetRemainder

```cpp
[UInt64](#sym-15243775785300097739) GetRemainder(const [System::FileHandle](#sym-15071417447436378148)& file_handle);
```

<a id="sym-9443082047660890175"></a>

#### File::GetSystemPath

```cpp
[WString](#sym-17787487981880441547) GetSystemPath([System::Path](#sym-8974154334980923428) path);
```

Returns the actual location of a system defined 'special' path.
Alias of System::GetPath.
<a id="sym-4497562553621289023"></a>

#### File::GetVolumes

```cpp
[Array < Pair < Array <WChar> , Array <WChar> > >](#sym-925399584115751627) GetVolumes();
```

<a id="sym-15415839624482920511"></a>

#### File::IsDirectory

```cpp
bool IsDirectory(const [WString](#sym-17787487981880441547)& path);
```

Returns true is path is a directory/folder.
This is an alias of System::IsDirectory.
<a id="sym-4686556267289909311"></a>

#### File::MakeDirectory

```cpp
bool MakeDirectory(const [WString](#sym-17787487981880441547)& path);
```

Creates a directory (folder) at the given path. Returns true on success, false on failure.
This is an alias of System::MakeDirectory.
<a id="sym-13203014036258691135"></a>

#### File::MakePath

```cpp
void MakePath(const [WString](#sym-17787487981880441547)& path);
```

<a id="sym-4205262043121588287"></a>

#### File::MakeRelativePath

```cpp
[WString](#sym-17787487981880441547) MakeRelativePath(const [WString::View](#sym-5727895666471172811)& base_dir, const [WString::View](#sym-5727895666471172811)& path);
```

Computes a relative path from 'base_dir' to 'path' using purely lexical (string) path comparison.
- base_dir: Base path used as the “from” location
- path: Target path to express relative to base_dir
- return: A relative path using ".." segments and System::kPathDelimiter. If either path cannot be split into components, returns path unchanged.

The result is formed by:
- finding the longest common prefix of the path components of base_dir to path
- emitting ".." for each remaining component in base_dir
- then appending the remaining components of path

- This is a lexical operation; it does not access the filesystem (i.e. does not resolve symlinks, current working directory, or check path existence).
- Input is assumed to be a valid Reflex directory path, i.e. terminated with System::kPathDelimiter

```cpp
auto rel = Reflex::File::MakeRelativePath(L"/a/b/c/", L"/a/b/d/e.txt");
// rel == "../d/e.txt"
```

```cpp
auto rel = Reflex::File::MakeRelativePath(L"/project/assets/", L"/project/assets/textures/wood.png");
// rel == "textures/wood.png"
```

<a id="sym-8974068045524899903"></a>

#### File::Open

```cpp
[Data::Archive](#sym-11560583731558276729) Open(const [WString](#sym-17787487981880441547)& path);
```

Reads the entire contents of a file into a Data::Archive.  Open is a convenience wrapper around System::FileHandle for cases where a file needs to be loaded into memory at once.
- filename: Path to the file to read.
- return: A Data::Archive containing the complete file contents. Empty if the file cannot be opened or read.

The archive is sized to the file length, and the file is read in a single operation.
```cpp
auto bytes = File::Open(filename);
if (bytes.GetSize() > 4)
{
	Data::Archive::View stream = bytes;
	if (Data::Deserialize<UInt32>(stream) == kMagicBytes)
	{
		//deserialize remaining data according to the expected format
	}
}
```

This helper is suitable for loading small–medium resources, configuration files, text files, and binary assets that fit comfortably in memory.
<a id="sym-8974170931466475583"></a>

#### File::Peek

```cpp
[Data::Archive](#sym-11560583731558276729) Peek([System::FileHandle](#sym-15071417447436378148)& file_handle, [UInt32](#sym-15243775351508400843) bytes);
```

<a id="sym-10445329438029781055"></a>

#### File::ReadBytes

```cpp
[Data::Archive](#sym-11560583731558276729) ReadBytes([System::FileHandle](#sym-15071417447436378148)& file_handle);
[Data::Archive](#sym-11560583731558276729) ReadBytes([System::FileHandle](#sym-15071417447436378148)& file_handle, [UInt32](#sym-15243775351508400843) bytes);
```

Reads raw bytes from a file into a Data::Archive.  Two overloads are provided: one that reads all remaining bytes, and one that reads up to a specified number of bytes.

### Overload 1

- file_handle: The file to read from. The read position advances as bytes are consumed.
- return: A Data::Archive containing all remaining bytes from the current file position to end of file.


### Overload 2

- file_handle: The file to read from.
- num_bytes: Maximum number of bytes to read. If fewer bytes remain in the file, only the remaining bytes are returned.
- return: A Data::Archive containing the requested byte range (or fewer, if EOF is reached).

```cpp
auto handle = Make<System::FileHandle>(path);

if (handle->GetSize() > 32)
{
	handle->SetPosition(16);
	auto region = File::ReadBytes(handle, 16);
	//use region
}
```

<a id="sym-8143884342183204927"></a>

#### File::ReadLine

```cpp
bool ReadLine([System::FileHandle](#sym-15071417447436378148)& file_handle, [CString](#sym-17531698610406914763)& line_out);
bool ReadLine([System::FileHandle](#sym-15071417447436378148)& file_handle, [WString](#sym-17787487981880441547)& line_out);
```

Reads a single line of text from the file and decodes it as either raw ASCII (CString overload) or UTF-8 (WString overload).
- file_handle: An open file handle positioned at the current read offset.
- line_out: Output buffer receiving the decoded line. CString reads raw ASCII bytes; WString decodes from UTF-8.
- return: true if a line was read (including empty lines); false only when no more lines are available (end-of-file).

```cpp
//read entire ASCII text file
auto file_handle = Make<System::FileHandle>(path);
CString buffer;
while (File::ReadLine(file_handle, buffer))
{
	//process line
}
```

<a id="sym-10543459708347488319"></a>

#### File::ReadValue

```cpp
bool ReadValue([System::FileHandle](#sym-15071417447436378148)& file_handle, TYPE& value_out);
TYPE ReadValue([System::FileHandle](#sym-15071417447436378148)& file_handle);
```

Reads a fixed-size value from the file into a raw-packable type. This is a low-level binary read intended for POD / trivially copyable types with a stable binary layout.
- file_handle: File to read from. The read position is advanced by sizeof(TYPE).
- inout: Destination value. On success it is overwritten with the bytes read. On failure it is unchanged or partially overwritten depending on underlying read behavior.
- return: true if exactly sizeof(TYPE) bytes were read, otherwise (insufficient bytes) false.

TYPE must be raw-packable (compile-time enforced). Non raw-packable types will fail to compile.
```cpp
auto file_handle = Make<System::FileHandle>(path);
UInt32 a = 0;
if (File::ReadValue(*file_handle, a))
{
	//use a
}

auto b = File::ReadValue<Float32>(*file_handle);
```

<a id="sym-14766849600538155071"></a>

#### File::RemoveDuplicateStrokes

```cpp
[WString](#sym-17787487981880441547) RemoveDuplicateStrokes(const [WString::View](#sym-5727895666471172811)& path);
```

<a id="sym-12916606465702238271"></a>

#### File::RemoveTrailingStroke

```cpp
[WString::View](#sym-5727895666471172811) RemoveTrailingStroke(const [WString::View](#sym-5727895666471172811)& path);
```

<a id="sym-14882056995832207423"></a>

#### File::Rename

```cpp
bool Rename(const [WString](#sym-17787487981880441547)& from, const [WString](#sym-17787487981880441547)& to);
```

<a id="sym-8927668057832298559"></a>

#### File::ResolveExistingFolder

```cpp
[WString::View](#sym-5727895666471172811) ResolveExistingFolder(const [WString::View](#sym-5727895666471172811)& path);
```

<a id="sym-7752503441585477695"></a>

#### File::ResolveRelativePath

```cpp
[WString](#sym-17787487981880441547) ResolveRelativePath(const [WString::View](#sym-5727895666471172811)& path);
```

<a id="sym-8974617651014932543"></a>

#### File::Save

```cpp
bool Save(const [WString](#sym-17787487981880441547)& filename, const [Data::Archive::View](#sym-3410220346311237241)& data);
```

Writes the contents of a byte buffer to a file.  Save is the inverse of File::Open: instead of loading the entire file into an Archive, it writes an Archive (or view of one) out to disk (typically).
- filename: Destination path to write.
- data: The byte range to write. Typically an Archive or Archive::View produced by serialization or binary packing.
- return: true on success, false if the file could not be opened or all bytes could not be written.

Save performs a single write of the provided data range. No additional formatting or metadata is added.
```cpp
Data::Archive stream;
Data::Serialize(stream, 123, 4.5f);
Data::SerializeUTF8(stream, L"hello");
if (!File::Save(L"output.bin", out))
{
	//handle save failure
}
```

<a id="sym-2572262434336282687"></a>

#### File::SplitExtension

```cpp
[Pair < ArrayView <WChar> , ArrayView <WChar> >](#sym-8974152822052584139) SplitExtension(const [WString::View](#sym-5727895666471172811)& path);
```

<a id="sym-8578058845219330111"></a>

#### File::SplitFilename

```cpp
[Pair < ArrayView <WChar> , ArrayView <WChar> >](#sym-8974152822052584139) SplitFilename(const [WString::View](#sym-5727895666471172811)& path);
```

<a id="sym-3286230485926287423"></a>

#### File::WriteBytes

```cpp
[UInt32](#sym-15243775351508400843) WriteBytes([System::FileHandle](#sym-15071417447436378148)& file_handle, const [Data::Archive::View](#sym-3410220346311237241)& bytes);
```

Writes the given byte view to the file at the current cursor position.
- file_handle: Open file handle to write into.
- bytes: Byte view to write.
- return: Number of bytes actually written.

A short write (return < bytes.GetSize()) indicates an I/O failure, full disk, or write restrictions.
<a id="sym-7367949705037355071"></a>

#### File::WriteLine

```cpp
void WriteLine([System::FileHandle](#sym-15071417447436378148)& file_handle, const [CString::View](#sym-2022350079943473867)& line);
void WriteLine([System::FileHandle](#sym-15071417447436378148)& file_handle, const [WString::View](#sym-5727895666471172811)& line);
```

Writes a single line of text to the file, encoding the input as either raw ASCII (CString::View overload) or UTF-8 (WString::View overload).
- file_handle: An open file handle positioned at the current write offset.
- line: The line to write. CString::View is written as raw ASCII bytes; WString::View is encoded to UTF-8.

A line terminator is automatically appended. The input string should not include a newline.
```cpp
auto file_handle = Make<System::FileHandle>(path);

//write plain ASCII
File::WriteLine(file_handle, "first line");
File::WriteLine(file_handle, "second line");

//write multibyte UTF-8
File::WriteLine(file_handle, L"UTF-8 text");
File::WriteLine(file_handle, L"日本語");
```

<a id="sym-3384360756243994687"></a>

#### File::WriteValue

```cpp
bool WriteValue([System::FileHandle](#sym-15071417447436378148)& file_handle, TYPE value);
```

Writes a single raw value directly to the file.  TYPE must be a raw-packable type - i.e., a type whose binary representation is stable, contiguous, and platform-independent.
- file_handle: The file to write to.
- in: The value to write. Written as its raw binary representation.
- return: true if the full value was written; false if the write failed or reached EOF.

WriteValue performs no encoding or metadata emission. The bytes of the value are written exactly as they exist in memory.
```cpp
auto f = Make<System::FileHandle>(path, File::kWrite);

UInt32 counter = 123;
File::WriteValue(f, counter);      //writes 4 bytes
```

```cpp
struct Foo
{
	UInt32 a;
	Float32 b;
};
Foo foo = { 10, 2.5f };
File::WriteValue(f, foo);          //OK if Foo is trivially copyable
```

<a id="sym-1931492178932144191"></a>

#### File::PersistentPropertySet

Inherits from [Data::PropertySet](#sym-13039646562789154425)
PersistentPropertySet brings together Data::PropertySet and Data::Format to create a PropertySet that can be stored and restored.
It is used primarily by the Bootstrap layer for the prefs object and the app session object, and it is the underlying mechanism behind the Bootstrap::Streamable persistence system used by the app, views, and other clients.
You can subscribe to changes by creating a listener using the Signal::CreateListener pattern.  The receiver must store the returned object to keep the connection alive.
See also
[Data::Format](#sym-12916641001534185081), [Signal](#sym-15069494894420785867)
<a id="sym-2784316003510786111"></a>

#### File::ResourcePool

Inherits from [Object](#sym-14361920786414140107)
ResourcePool is the higher-level shared resource cache built on top of the file layer.
It resolves resources by address or path, ensures the underlying file is decoded only once, and then keeps the in-memory representation available for reuse across the application.
In practice this is used extensively by GLX for stylesheets, fonts, bitmaps, and other UI resources, but it was originally designed around audio/sample loading and is equally well proven there.
ResourcePool typically sits above [File::VirtualFileSystem](#sym-3007734335323379775), which handles the lower-level path resolution.
<a id="sym-3007734335323379775"></a>

#### File::VirtualFileSystem

Inherits from [Object](#sym-14361920786414140107)
VirtualFileSystem is the lower-level path resolution layer used to map a path onto a concrete file source.
Rather than being limited to disk files, it works through attachable locator handlers for different domains and storage types.  This allows special paths such as ":res:Project/styles.glx" to resolve through the appropriate handler instead of directly through the OS filesystem.
In practice this is used for embedded resources, monolithic containers, web-backed content, and other custom storage schemes.  Most application code uses higher-level APIs on top of it rather than talking to the VirtualFileSystem directly.
---


##### [Reflex](#sym-14880872606059205893) > GLX

<a id="sym-11040927282821694730"></a>

#### GLX::kCharacter

```cpp
const kCharacter kCharacter;
```

<a id="sym-7745368244894139658"></a>

#### GLX::kDragDropEnter

```cpp
const kDragDropEnter kDragDropEnter;
```

<a id="sym-7779547040584926474"></a>

#### GLX::kDragDropLeave

```cpp
const kDragDropLeave kDragDropLeave;
```

<a id="sym-1266180577300677898"></a>

#### GLX::kDragDropReceive

```cpp
const kDragDropReceive kDragDropReceive;
```

<a id="sym-7557535242487050"></a>

#### GLX::kDragDropReceiveExternal

```cpp
const kDragDropReceiveExternal kDragDropReceiveExternal;
```

<a id="sym-18263983963024491786"></a>

#### GLX::kDragDropTender

```cpp
const kDragDropTender kDragDropTender;
```

<a id="sym-479710398262641930"></a>

#### GLX::kFocus

```cpp
const kFocus kFocus;
```

<a id="sym-13606745366836905226"></a>

#### GLX::kKeyDown

```cpp
const kKeyDown kKeyDown;
```

<a id="sym-503732725874787594"></a>

#### GLX::kKeyUp

```cpp
const kKeyUp kKeyUp;
```

<a id="sym-8127020559235122442"></a>

#### GLX::kLoseFocus

```cpp
const kLoseFocus kLoseFocus;
```

<a id="sym-11672120847782479114"></a>

#### GLX::kMouseDown

```cpp
const kMouseDown kMouseDown;
```

<a id="sym-11672131731229607178"></a>

#### GLX::kMouseDrag

```cpp
const kMouseDrag kMouseDrag;
```

<a id="sym-16250030719408112906"></a>

#### GLX::kMouseEnter

```cpp
const kMouseEnter kMouseEnter;
```

<a id="sym-16284209515098899722"></a>

#### GLX::kMouseLeave

```cpp
const kMouseLeave kMouseLeave;
```

<a id="sym-7921307822078395658"></a>

#### GLX::kMouseUp

```cpp
const kMouseUp kMouseUp;
```

<a id="sym-16340717300300743946"></a>

#### GLX::kMouseWheel

```cpp
const kMouseWheel kMouseWheel;
```

<a id="sym-9428781797547803914"></a>

#### GLX::kTransaction

"Transaction" / kTransaction is a standard event used by interactive widgets to report value changes in a structured begin -> perform -> end sequence. It is emitted during continuous user interactions such as dragging, scrolling, typing, and other adjustments where you typically want live updates plus a single undo/commit at the end.
Widgets that emit transactions include RotarySlider, RangeBar, and TextArea.
The transaction stage is stored on the event and can be read using GLX::GetTransactionStage(e).
- kTransactionStageBegin: interaction started (e.g. mouse down, drag begin, edit begin)
- kTransactionStagePerform: value changed during the interaction (may be emitted many times)
- kTransactionStageEnd: interaction finished (e.g. mouse up, drag end, edit end)
- kTransactionStageCancel: interaction aborted (e.g. escape, focus loss, cancelled drag)


#### Event properties

The following properties may be attached to the event:
- stage: UInt8 (TransactionStage). Always set.
- index: UInt32. Optional. Used when a widget reports multiple lanes/handles/fields.
- value: Float32. Optional. The current value for the interaction step.
- modifiers: UInt8. Modifier key state at emit time (System::GetModifierKeys()).

index and value are optional; some widgets emit only the stage (and modifiers) and expect the receiver to query widget state directly.

#### Handling Transactions

Typical handling is to bind to GLX::kTransaction, switch on the stage, apply live updates on kTransactionStagePerform, and push a single undo/commit on kTransactionStageEnd.
```cpp
GLX::BindEvent(widget, GLX::kTransaction, [](GLX::Object & src, GLX::Event & e)
{
	auto stage = GLX::GetTransactionStage(e);

	switch (stage)
	{
		case GLX::kTransactionStageBegin:
		{
			//optional: capture initial state for undo / preview
			break;
		}

		case GLX::kTransactionStagePerform:
		{
			//live update (engine/app state)
			//optional: use GLX::GetIndex(e) / GLX::GetValue(e) if provided
			break;
		}

		case GLX::kTransactionStageEnd:
		{
			//commit once (e.g. add undo step)
			break;
		}

		case GLX::kTransactionStageCancel:
		{
			//optional: revert to initial state
			break;
		}

		default: break;
	}

	return true;
});
```


#### Emission

Transactions are typically emitted via the helper EmitTransaction

#### Notes

- kTransactionStagePerform may be emitted at high frequency; handlers should be lightweight.
- If index/value are not present, read the current state from src (or your model) instead.
- Use modifiers to support alternate modes (fine adjust, snapping, multi-select, etc.).

```cpp
const kTransaction kTransaction;
```

<a id="sym-17523243275744544010"></a>

#### GLX::FlowFlags

```cpp
kFlowX
kFlowY
kFlowInvert
kFlowCenter
```

<a id="sym-429781442101347594"></a>

#### GLX::Orientation

```cpp
kOrientationNear
kOrientationCenter
kOrientationFar
kOrientationFit
```

<a id="sym-13575353154517697802"></a>

#### GLX::Alignment

```cpp
kAlignmentTopLeft
kAlignmentTop
kAlignmentTopRight
kAlignmentLeft
kAlignmentCenter
kAlignmentRight
kAlignmentBottomLeft
kAlignmentBottom
kAlignmentBottomRight
```

<a id="sym-1404815326977295626"></a>

#### GLX::ClickFlags

```cpp
kClickFlagRmb
kClickFlagDbl
```

<a id="sym-12677384309400002985"></a>

#### InterpolatedAnimation::Easing

```cpp
kLinear
kEaseIn2x
kEaseIn3x
kEaseOut2x
kEaseOut3x
kEaseInOutCos
kEaseInOut2x
```

<a id="sym-3412323203366880522"></a>

#### GLX::TransactionStage

```cpp
kTransactionStageNone
kTransactionStageBegin
kTransactionStagePerform
kTransactionStageEnd
kTransactionStageCancel
```

<a id="sym-832412837035859918"></a>

#### AbstractList::SelectionMode

```cpp
kSelectionModeSingle
kSelectionModeMulti
kSelectionModeMultiToggle
```

<a id="sym-12411471395882632458"></a>

#### GLX::Colour

```cpp
using Colour = [System::Colour](#sym-12411471392737976356);
```

<a id="sym-3902131130752664842"></a>

#### GLX::ColourPoints

```cpp
using ColourPoints = [Array < Tuple < Point <Float32> ,System::Colour> >](#sym-925399584115751627);
```

<a id="sym-9609741839313765642"></a>

#### GLX::KeyCode

```cpp
using KeyCode = [System::KeyCode](#sym-9609741836169109540);
```

<a id="sym-16781546979716072714"></a>

#### GLX::ModifierKeys

Convenience alias of System::ModifierKeys. Use GLX::kModifierKeyPrimary to identify Ctrl (Windows) or Cmd (macOS).
```cpp
if (GLX::GetModifierKeys(e) & GLX::kModifierKeyShift)
{
	//handle shift
}
```

```cpp
using ModifierKeys = [System::ModifierKeys](#sym-16781546976571416612);
```

<a id="sym-3088564088943510794"></a>

#### GLX::MouseCursor

```cpp
using MouseCursor = [System::MouseCursor](#sym-3088564085798854692);
```

<a id="sym-1001298644097402122"></a>

#### GLX::Point

```cpp
using Point = [System::fPoint](#sym-18136990298152230948);
```

<a id="sym-14596111566169539850"></a>

#### GLX::Points

```cpp
using Points = [Array < Point <Float32> >](#sym-925399584115751627);
```

<a id="sym-8974479385545508106"></a>

#### GLX::Rect

```cpp
using Rect = [System::fRect](#sym-1108859096084373540);
```

<a id="sym-8974655638118434058"></a>

#### GLX::Size

```cpp
using Size = [System::fSize](#sym-1109035348657299492);
```

<a id="sym-6013620891227882762"></a>

#### GLX::Activate

```cpp
void Activate([Object](#sym-14361920786363680010)& object, bool state);
```

<a id="sym-13371338634544907530"></a>

#### GLX::ActivateBranch

```cpp
void ActivateBranch([Object](#sym-14361920786363680010)& object, bool state);
```

<a id="sym-7316673660566209802"></a>

#### GLX::AddAbsolute

```cpp
[TRef <Object>](#sym-8974699438245378763) AddAbsolute([Object](#sym-14361920786363680010)& parent, [Object](#sym-14361920786363680010)& child);
[TRef <Object>](#sym-8974699438245378763) AddAbsolute([Object](#sym-14361920786363680010)& parent, [Object](#sym-14361920786363680010)& child, [Point](#sym-1001298644097402122) position);
```

Places the child at a fixed position within the parent's content rect, without using the layout system.
This function should generally not be used in application code, typical use-cases are for building complex widgets (such as ViewPort).
The real power of the layout system comes from using inline and float modes, which provide dynamic, responsive layouts.  AddAbsolute bypasses those mechanisms and should be considered a low-level escape hatch.
<a id="sym-4770899402942416138"></a>

#### GLX::AddDottedLine

```cpp
void AddDottedLine([Points](#sym-14596111566169539850)& points, [Point](#sym-1001298644097402122) from, [Point](#sym-1001298644097402122) to, [Size](#sym-8974655638118434058) pixel_size);
```

<a id="sym-12312432654082475274"></a>

#### GLX::AddEllipseFill

```cpp
void AddEllipseFill([Points](#sym-14596111566169539850)& points, const [Rect](#sym-8974479385545508106)& rect, [Float32](#sym-1452730841175390923) start, [Float32](#sym-1452730841175390923) sweep);
```

<a id="sym-4928174929158964490"></a>

#### GLX::AddEllipseOutline

```cpp
void AddEllipseOutline([Points](#sym-14596111566169539850)& points, const [Rect](#sym-8974479385545508106)& rect, [Size](#sym-8974655638118434058) width, [Float32](#sym-1452730841175390923) start, [Float32](#sym-1452730841175390923) sweep);
```

<a id="sym-8691317006378566922"></a>

#### GLX::AddFloat

```cpp
[TRef <Object>](#sym-8974699438245378763) AddFloat([Object](#sym-14361920786363680010)& parent, [Object](#sym-14361920786363680010)& child, [Orientation](#sym-429781442101347594) x_position, [Orientation](#sym-429781442101347594) y_position);
[TRef <Object>](#sym-8974699438245378763) AddFloat([Object](#sym-14361920786363680010)& parent, [Object](#sym-14361920786363680010)& child, [Alignment](#sym-13575353154517697802) alignment);
```

Adds child to parent with float positioning, independent of the inline flow.  The childs is positioned either by specifying Orientation values for both axes, or by using a single Alignment value as shorthand.
In the shorthand form, the alignment parameters selects one of nine standard positions (top-left, top etc...)
In the full form, Orientation gives fine-grained control along X and Y:
- kOrientationNear/Center/Far position the child relative to the parent on that axis,
- kOrientationFit stretches the child to fit that axis.

Common patterns include:
- x=Fit, y=Fit: full stretch overlay across the parent.
- x=Far, y=Fit: sticky footer that stays pinned to the bottom edge.
- alignment=TopRight: floating badge in the top right corner.
- alignment=Center: centered modal or dialog.

AddFloat does not consume inline space - it layers the child over the parent's content area.  Combine with margin and parent padding for refined placement.
<a id="sym-10626316634991723786"></a>

#### GLX::AddInline

```cpp
[TRef <Object>](#sym-8974699438245378763) AddInline([Object](#sym-14361920786363680010)& parent, [Object](#sym-14361920786363680010)& child, [Orientation](#sym-429781442101347594) ortho_position);
```

Adds child to parent with inline positioning (horizontal or vertical depending on parent layout). The child is positioned on the parents ortho-axis according to the orientation paremeter.
- parent: Container receiving the child.
- child: Element to insert into the inline sequence.
- orientation: kOrientationNear/Center/Far/Fit along the ortho axis (default kOrientationFit)

<a id="sym-5056430349198918922"></a>

#### GLX::AddInlineFlex

```cpp
[TRef <Object>](#sym-8974699438245378763) AddInlineFlex([Object](#sym-14361920786363680010)& parent, [Object](#sym-14361920786363680010)& child, [Orientation](#sym-429781442101347594) ortho_position);
```

Adds child to parent with inline positioning, similar to AddInline, but the child expands to fill available space along the main axis.  When multiple flex children are present, the available space is distributed evenly among them.
- parent: Container receiving the child.
- child: Element to insert into the inline sequence.
- orientation: kOrientationNear/Center/Far/Fit along the ortho axis (default kOrientationFit).

<a id="sym-9208741917075014922"></a>

#### GLX::AddPath

```cpp
void AddPath([Points](#sym-14596111566169539850)& points, const [ArrayView < Point <Float32> >](#sym-17111432006044187339)& path, bool& closed);
```

<a id="sym-12894796375077782794"></a>

#### GLX::AddPointsWithColour

```cpp
void AddPointsWithColour([ColourPoints](#sym-3902131130752664842)& colour_points, const [ArrayView < Point <Float32> >](#sym-17111432006044187339)& colour, const [Colour](#sym-12411471395882632458)& input);
```

<a id="sym-8690098717430220042"></a>

#### GLX::AddPolygonFill

```cpp
void AddPolygonFill([Points](#sym-14596111566169539850)& points, const [ArrayView < Point <Float32> >](#sym-17111432006044187339)& input);
```

<a id="sym-15549464018048615690"></a>

#### GLX::AddRectFill

```cpp
void AddRectFill([Points](#sym-14596111566169539850)& points, const [Rect](#sym-8974479385545508106)& rect);
void AddRectFill([ColourPoints](#sym-3902131130752664842)& colour_points, const [Colour](#sym-12411471395882632458)& colour, const [Rect](#sym-8974479385545508106)& rect);
```

<a id="sym-8956172967914603786"></a>

#### GLX::AddRectOutline

```cpp
void AddRectOutline([Points](#sym-14596111566169539850)& points, const [Rect](#sym-8974479385545508106)& rect, const [Margin](#sym-14021901793341048074)& width, [Size](#sym-8974655638118434058) pixel_size);
void AddRectOutline([ColourPoints](#sym-3902131130752664842)& colour_points, const [Colour](#sym-12411471395882632458)& colour, const [Rect](#sym-8974479385545508106)& rect, const [Margin](#sym-14021901793341048074)& width, [Size](#sym-8974655638118434058) pixel_size);
```

<a id="sym-8495876898985968906"></a>

#### GLX::AddRoundedFill

```cpp
void AddRoundedFill([Points](#sym-14596111566169539850)& points, const [Rect](#sym-8974479385545508106)& rect, const [Size](#sym-8974655638118434058)* corners[4], [Float32](#sym-1452730841175390923) corner_step);
void AddRoundedFill([Points](#sym-14596111566169539850)& points, const [Rect](#sym-8974479385545508106)& rect, const [Margin](#sym-14021901793341048074)& corners, [Float32](#sym-1452730841175390923) corner_step);
```

<a id="sym-906192056525882634"></a>

#### GLX::AddRoundedOutline

```cpp
void AddRoundedOutline([Points](#sym-14596111566169539850)& points, const [Rect](#sym-8974479385545508106)& rect, const [Margin](#sym-14021901793341048074)& width, const [Size](#sym-8974655638118434058)* corners[4], [Float32](#sym-1452730841175390923) corner_step);
void AddRoundedOutline([Points](#sym-14596111566169539850)& points, const [Rect](#sym-8974479385545508106)& rect, const [Margin](#sym-14021901793341048074)& width, const [Margin](#sym-14021901793341048074)& corners, [Float32](#sym-1452730841175390923) corner_step);
```

<a id="sym-18110915687986857226"></a>

#### GLX::AddRoundedTriangleFill

```cpp
void AddRoundedTriangleFill([Points](#sym-14596111566169539850)& points, const [Rect](#sym-8974479385545508106)& rect, [Float32](#sym-1452730841175390923) corner, [Alignment](#sym-13575353154517697802) direction, [Size](#sym-8974655638118434058) pixel_size);
```

<a id="sym-10591907727838119178"></a>

#### GLX::AddRoundedTriangleOutline

```cpp
void AddRoundedTriangleOutline([Points](#sym-14596111566169539850)& points, const [Rect](#sym-8974479385545508106)& rect, [Float32](#sym-1452730841175390923) width, [Float32](#sym-1452730841175390923) corner, [Alignment](#sym-13575353154517697802) direction, [Alignment](#sym-13575353154517697802) pixel_size);
```

<a id="sym-1346689314062435594"></a>

#### GLX::AddStretch

```cpp
[TRef <Object>](#sym-8974699438245378763) AddStretch([Object](#sym-14361920786363680010)& parent, [Object](#sym-14361920786363680010)& child);
```

Shorthand for AddFloat with both axes set to kOrientationFit.
Adds the child stretched to fill the entire parent's content area.  Useful for overlays, background layers, or full-size panels where the child should always match the parent's size.
<a id="sym-17532202238221583626"></a>

#### GLX::AddTriangleFill

```cpp
void AddTriangleFill([Points](#sym-14596111566169539850)& points, const [Rect](#sym-8974479385545508106)& rect, [Alignment](#sym-13575353154517697802) direction);
```

<a id="sym-2847234583865427210"></a>

#### GLX::AddTriangleOutline

```cpp
void AddTriangleOutline([Points](#sym-14596111566169539850)& points, const [Rect](#sym-8974479385545508106)& rect, [Float32](#sym-1452730841175390923) width, [Alignment](#sym-13575353154517697802) direction, [Size](#sym-8974655638118434058) pixel_size);
```

<a id="sym-15187249853810967818"></a>

#### GLX::AttachAnimationClock

```cpp
void AttachAnimationClock([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id, const [Function <void(Float32)>](#sym-5470074934380427979)& callback);
```

<a id="sym-15145119518877123850"></a>

#### GLX::AttachPeriodicClock

```cpp
void AttachPeriodicClock([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id, [Float32](#sym-1452730841175390923) interval, const [Function <void()>](#sym-5470074934380427979)& callback);
```

<a id="sym-16354089260070044938"></a>

#### GLX::BindClick

```cpp
[TRef <Object>](#sym-8974699438245378763) BindClick([Object](#sym-14361920786363680010)& object, const [Function <void()>](#sym-5470074934380427979)& callback);
```

<a id="sym-16365802615138780426"></a>

#### GLX::BindEvent

```cpp
void BindEvent([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) event_id, const [Function <bool(Object&, Event&)>](#sym-5470074934380427979)& callback);
```

Attaches a delegate to the specified object which filters and forwards only events matching the given event-id to the supplied callback.
- object: Target object that emits events.
- event_id: Event ID (e.g. GLX::kMouseDown).
- callback: Function called when the event occurs.

BindEvent replaces any previous delegate with the same event_id on this object.
The callback should return true if the event is fully handled, to stop propagation, or false to allow further delegates to receive it.
The following example shows how to intercept and trap mouse clicks. (BindClick is a shorthand for this).
```cpp
GLX::BindEvent(button, GLX::Button::kMouseDown, [](GLX::Object & src, GLX::Event & e)
{
	Log("Button clicked!");
	return true; // stop propagation
});
```

<a id="sym-10149938692383409418"></a>

#### GLX::BindEventVoid

```cpp
void BindEventVoid([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) event_id, const [Function <void()>](#sym-5470074934380427979)& callback);
```

Convenience version of BindEvent for simple handlers.
- object: Target GLX::Object that emits the event.
- event_id: Event id to listen for.
- callback: Function(void) executed when the event triggers.

Invokes the supplied callback when the specified event-id occurs, and always traps the event.
The callback receives no parameters and does not see the source object or event payload.
<a id="sym-12012218215368328458"></a>

#### GLX::BranchContains

```cpp
bool BranchContains(const [Object](#sym-14361920786363680010)& parent, const [Object](#sym-14361920786363680010)& child);
```

<a id="sym-14943435499698750730"></a>

#### GLX::CalculateAbs

```cpp
[Pair < Point <Float32> , Size <Float32> >](#sym-8974152822052584139) CalculateAbs(const [Object](#sym-14361920786363680010)& object);
[Pair < Point <Float32> , Size <Float32> >](#sym-8974152822052584139) CalculateAbs(const [Object](#sym-14361920786363680010)& parent, const [Object](#sym-14361920786363680010)& object);
```

<a id="sym-12186597521128260874"></a>

#### GLX::CalculateAbsoluteRect

```cpp
[Rect](#sym-8974479385545508106) CalculateAbsoluteRect(const [Object](#sym-14361920786363680010)& object);
```

<a id="sym-14502939204234872074"></a>

#### GLX::CalculateRelativeRect

```cpp
[Rect](#sym-8974479385545508106) CalculateRelativeRect(const [Object](#sym-14361920786363680010)& parent, const [Object](#sym-14361920786363680010)& object);
```

<a id="sym-17000498287842788618"></a>

#### GLX::CancelDragDrop

```cpp
void CancelDragDrop();
```

<a id="sym-7183523781695079690"></a>

#### GLX::ClearText

```cpp
void ClearText([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id_opt);
```

<a id="sym-13586872535978444042"></a>

#### GLX::CloseContextMenu

```cpp
void CloseContextMenu();
```

<a id="sym-18141271455978063114"></a>

#### GLX::CreateAnimationClock

```cpp
[TRef <Object>](#sym-8974699438245378763) CreateAnimationClock(const [Function <void(Float32)>](#sym-5470074934380427979)& callback);
```

<a id="sym-16203876171916608778"></a>

#### GLX::CreateCallbackAnimation

```cpp
[TRef <Animation>](#sym-8974699438245378763) CreateCallbackAnimation(const [Function <void(Object&)>](#sym-5470074934380427979)& callback);
```

<a id="sym-14829502906532726026"></a>

#### GLX::CreateColourPropertyAnimation

```cpp
[TRef <InterpolatedAnimation>](#sym-8974699438245378763) CreateColourPropertyAnimation([Key32](#sym-974353891938499275) property_id, const [Colour](#sym-12411471395882632458)& from, const [Colour](#sym-12411471395882632458)& to);
```

<a id="sym-656786913419691274"></a>

#### GLX::CreateDragDropBeginListener

```cpp
[TRef <Object>](#sym-8974699438245378763) CreateDragDropBeginListener(const [Function <void(Object&)>](#sym-5470074934380427979)& callback);
```

<a id="sym-14306163233164199178"></a>

#### GLX::CreateDragDropEndListener

```cpp
[TRef <Object>](#sym-8974699438245378763) CreateDragDropEndListener(const [Function <void()>](#sym-5470074934380427979)& callback);
```

<a id="sym-8221730798634632458"></a>

#### GLX::CreateDragDropTargetListener

```cpp
[TRef <Object>](#sym-8974699438245378763) CreateDragDropTargetListener(const [Function <void(Object&)>](#sym-5470074934380427979)& callback);
```

<a id="sym-461854346607428874"></a>

#### GLX::CreateFloatPropertyAnimation

```cpp
[TRef <InterpolatedAnimation>](#sym-8974699438245378763) CreateFloatPropertyAnimation([Key32](#sym-974353891938499275) property_id, [Float32](#sym-1452730841175390923) from, [Float32](#sym-1452730841175390923) to);
```

<a id="sym-4822352329464448266"></a>

#### GLX::CreateInterpolatedAnimation

```cpp
[TRef <InterpolatedAnimation>](#sym-8974699438245378763) CreateInterpolatedAnimation(const [Function <void(Object&, Float32)>](#sym-5470074934380427979)& callback);
```

<a id="sym-13538936539131774218"></a>

#### GLX::CreateLogarithmicAnimation

```cpp
[TRef <Animation>](#sym-8974699438245378763) CreateLogarithmicAnimation([Float32](#sym-1452730841175390923) from, [Float32](#sym-1452730841175390923) to, const [Function <void(Object&, Float32)>](#sym-5470074934380427979)& callback, [Float32](#sym-1452730841175390923) decay_factor);
```

<a id="sym-15922530894628357386"></a>

#### GLX::CreateMarginPropertyAnimation

```cpp
[TRef <InterpolatedAnimation>](#sym-8974699438245378763) CreateMarginPropertyAnimation([Key32](#sym-974353891938499275) property_id, const [Margin](#sym-14021901793341048074)& from, const [Margin](#sym-14021901793341048074)& to);
```

<a id="sym-10471632103631062282"></a>

#### GLX::CreateMaxBoundsAnimation

```cpp
[TRef <Animation>](#sym-8974699438245378763) CreateMaxBoundsAnimation([Key32](#sym-974353891938499275) bounds_id, bool yaxis, [Float32](#sym-1452730841175390923) from, [Float32](#sym-1452730841175390923) to);
```

<a id="sym-4438300812032181514"></a>

#### GLX::CreateOpacityAnimation

```cpp
[TRef <InterpolatedAnimation>](#sym-8974699438245378763) CreateOpacityAnimation([Object](#sym-14361920786363680010)& target, [Key32](#sym-974353891938499275) id, [Float32](#sym-1452730841175390923) from, [Float32](#sym-1452730841175390923) to);
```

<a id="sym-16352619814319129866"></a>

#### GLX::CreatePeriodicClock

```cpp
[TRef <Object>](#sym-8974699438245378763) CreatePeriodicClock([Float32](#sym-1452730841175390923) interval, const [Function <void()>](#sym-5470074934380427979)& callback);
```

<a id="sym-11994997758743381258"></a>

#### GLX::CreatePointPropertyAnimation

```cpp
[TRef <InterpolatedAnimation>](#sym-8974699438245378763) CreatePointPropertyAnimation([Key32](#sym-974353891938499275) property_id, [Point](#sym-1001298644097402122) from, [Point](#sym-1001298644097402122) to);
```

<a id="sym-3372461435799045386"></a>

#### GLX::CreatePositionAnimation

```cpp
[TRef <InterpolatedAnimation>](#sym-8974699438245378763) CreatePositionAnimation(bool y, [Float32](#sym-1452730841175390923) from, [Float32](#sym-1452730841175390923) to);
```

<a id="sym-1956205431755998474"></a>

#### GLX::CreateSizePropertyAnimation

```cpp
[TRef <InterpolatedAnimation>](#sym-8974699438245378763) CreateSizePropertyAnimation([Key32](#sym-974353891938499275) property_id, [Size](#sym-8974655638118434058) from, [Size](#sym-8974655638118434058) to);
```

<a id="sym-14630098535735985418"></a>

#### GLX::CreateStateAnimation

```cpp
[TRef <Animation>](#sym-8974699438245378763) CreateStateAnimation([Key32](#sym-974353891938499275) state);
```

<a id="sym-3214050947294659850"></a>

#### GLX::CreateWaitAnimation

```cpp
[TRef <InterpolatedAnimation>](#sym-8974699438245378763) CreateWaitAnimation();
```

<a id="sym-18005836590080951562"></a>

#### GLX::DetachClock

```cpp
void DetachClock([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id);
```

<a id="sym-8972511126587802890"></a>

#### GLX::Emit

```cpp
bool Emit([Object](#sym-14361920786363680010)& src, [Key32](#sym-974353891938499275) id, VARGS... id_value_pairs);
```

Posts a custom event upward from the specified object.  This is the recommended helper for emitting events without manually constructing an Event object.
- src: The object emitting the event. The event is dispatched upward from this object.
- id: The Key32 event identifier.
- id_value_pairs: A variadic list of key/value pairs written into the event’s data payload. Keys are identifiers; values may be any supported data type.
- return: boolean indicating whether the event was trapped or not

```cpp
Emit(object, "my_event", "value", 1.0f);
```

The call above is equivalent to:
```cpp
auto e = Make<Event>("my_event");
Data::SetFloat(e, "value", 1.0f);
object.Emit(e);
```

This helper is intended for simple transactional or notification-style events. For advanced cases (preconfigured payloads or reuse), construct and emit an Event explicitly.
<a id="sym-17128905620876002570"></a>

#### GLX::EnableAutoFit

```cpp
void EnableAutoFit([Object](#sym-14361920786363680010)& object, bool x, bool y);
```

<a id="sym-11035566029764920586"></a>

#### GLX::EnableMouse

```cpp
void EnableMouse([Object](#sym-14361920786363680010)& object, bool enable, bool intercept);
```

Controls how an object participates in mouse hit-testing.
- Object obj: target object
- bool enable: allows mouse events on the target (default true)
- bool intercept: prevents children from receiving mouse events (default false)


### Common combinations:

```cpp
//obj receives mouse events, if a child doesnt have priorty (default behaviour)
GLX::EnableMouse(obj, true);

//obj receives all mouse events, stealing from children
GLX::EnableMouse(obj, true, true);

//obj is excluded from hit-testing, but its children can still receive mouse events normally
GLX::EnableMouse(obj, false)

//the object and its entire subtree are invisible to the mouse
GLX::EnableMouse(obj, false, true)

```

<a id="sym-1466049126687933706"></a>

#### GLX::EnableMouseCapture

```cpp
void EnableMouseCapture([Object](#sym-14361920786363680010)& object, bool enable, bool incremental);
```

<a id="sym-945166050058667274"></a>

#### GLX::Enter

```cpp
void Enter([Object](#sym-14361920786363680010)& object, [UInt8](#sym-1020924849394252491) flags);
```

<a id="sym-4967958786020050186"></a>

#### GLX::ExceedsDragThreshold

```cpp
bool ExceedsDragThreshold([Point](#sym-1001298644097402122) drag, [Float32](#sym-1452730841175390923) sens);
```

<a id="sym-8972562576001041674"></a>

#### GLX::Exit

```cpp
void Exit([Object](#sym-14361920786363680010)& object, bool detach, [UInt8](#sym-1020924849394252491) or_flags_opt);
```

<a id="sym-13118240072967226634"></a>

#### GLX::FindStyle

```cpp
[ConstTRef <Style>](#sym-532369501975706315) FindStyle(const [Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id);
[ConstTRef <Style>](#sym-532369501975706315) FindStyle(const [Style](#sym-1017425348645717258)& style, [Key32](#sym-974353891938499275) id);
```

Finds a style by path relative to 'start'.
Resolution order:
- 1. Direct child of 'start'.
- 2. If not found, perform a fallback search "upwards" through the stylesheet (older-siblings and parents).

Returns null if no match is found.
<a id="sym-6931605747713279242"></a>

#### GLX::FocusBranch

```cpp
void FocusBranch([Object](#sym-14361920786363680010)& branch_root);
```

<a id="sym-18017645576891499786"></a>

#### GLX::GetBounds

```cpp
const [Pair < Size <Float32> , Size <Float32> >](#sym-8974152822052584139)& GetBounds(const [Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id);
```

<a id="sym-877631799612769546"></a>

#### GLX::GetClickFlags

```cpp
[UInt8](#sym-1020924849394252491) GetClickFlags(const [Event](#sym-946331961880839434)& e);
```

<a id="sym-5843754414980039946"></a>

#### GLX::GetClip

```cpp
[Pair <bool,bool>](#sym-8974152822052584139) GetClip(const [Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id);
```

<a id="sym-1676901685567522058"></a>

#### GLX::GetDragDropData

```cpp
[TRef <Object>](#sym-8974699438245378763) GetDragDropData([Event](#sym-946331961880839434)& e);
```

<a id="sym-14581210129891427594"></a>

#### GLX::GetKeyCharacter

```cpp
[WChar](#sym-1030155236624530123) GetKeyCharacter(const [Event](#sym-946331961880839434)& e);
```

<a id="sym-15647254058237003018"></a>

#### GLX::GetKeyCode

```cpp
[KeyCode](#sym-9609741839313765642) GetKeyCode(const [Event](#sym-946331961880839434)& e);
```

<a id="sym-14527751964743271690"></a>

#### GLX::GetModifierKeys

```cpp
[UInt8](#sym-1020924849394252491) GetModifierKeys(const [Event](#sym-946331961880839434)& e);
```

<a id="sym-9233775494737003786"></a>

#### GLX::GetMousePosition

```cpp
[Point](#sym-1001298644097402122) GetMousePosition(const [Object](#sym-14361920786363680010)& object);
```

<a id="sym-2672623865267586314"></a>

#### GLX::GetOpacity

```cpp
[Float32](#sym-1452730841175390923) GetOpacity(const [Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id);
```

<a id="sym-5846347737708201226"></a>

#### GLX::GetText

```cpp
[WString::View](#sym-5727895666471172811) GetText(const [Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id_opt);
```

<a id="sym-991493259530437898"></a>

#### GLX::IsActive

```cpp
bool IsActive(const [Object](#sym-14361920786363680010)& object);
```

<a id="sym-5896420424950056202"></a>

#### GLX::IsDoubleClick

```cpp
bool IsDoubleClick(const [Event](#sym-946331961880839434)& e);
```

<a id="sym-5604517198925235466"></a>

#### GLX::IsLeftClick

```cpp
bool IsLeftClick(const [Event](#sym-946331961880839434)& e);
```

<a id="sym-12740634717330703626"></a>

#### GLX::IsRightClick

```cpp
bool IsRightClick(const [Event](#sym-946331961880839434)& e);
```

<a id="sym-12390270548251346186"></a>

#### GLX::IsSelected

```cpp
bool IsSelected(const [Object](#sym-14361920786363680010)& object);
```

<a id="sym-3350518443588814090"></a>

#### GLX::LookupBranchIndex

```cpp
[Idx](#sym-830904007181695691) LookupBranchIndex(const [Object](#sym-14361920786363680010)& parent, const [Object](#sym-14361920786363680010)& child);
```

<a id="sym-14192400093637870858"></a>

#### GLX::LookupChildAtIndex

```cpp
[TRef <Object>](#sym-8974699438245378763) LookupChildAtIndex([Object](#sym-14361920786363680010)& parent, [UInt32](#sym-15243775351508400843) idx);
```

<a id="sym-13692084125035496714"></a>

#### GLX::LookupIndex

```cpp
[Idx](#sym-830904007181695691) LookupIndex(const [Object](#sym-14361920786363680010)& child);
```

<a id="sym-9476589778599249162"></a>

#### GLX::OpenContextMenu

```cpp
[Reference <Menu>](#sym-1695362219560368843) OpenContextMenu([Object](#sym-14361920786363680010)& src, [Key32](#sym-974353891938499275) context_opt, [Key32](#sym-974353891938499275) style_opt);
```

Attaches a Menu widget to the window foreground.
See Menu for more details.
<a id="sym-16990406935763322122"></a>

#### GLX::QueryAntecedent

```cpp
const [Event](#sym-946331961880839434)* QueryAntecedent(const [Event](#sym-946331961880839434)& e, [Key32](#sym-974353891938499275) id, const [Event](#sym-946331961880839434)* fallback);
```

<a id="sym-13513791923680281866"></a>

#### GLX::QueryChildById

```cpp
[Object](#sym-14361920786363680010)* QueryChildById([Object](#sym-14361920786363680010)& parent, [Key32](#sym-974353891938499275) id, [Object](#sym-14361920786363680010)* fallback);
```

Searches 'object' for the first direct child (i.e. no recursion) with the given 'id'.
<a id="sym-12831746998201845002"></a>

#### GLX::QueryDragDropData

```cpp
TYPE* QueryDragDropData([Event](#sym-946331961880839434)& e);
```

<a id="sym-12767373087771559178"></a>

#### GLX::QueryElementById

```cpp
[Object](#sym-14361920786363680010)* QueryElementById([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id, [Object](#sym-14361920786363680010)* fallback);
```

Searches 'object' recursively for the first child with the given 'id'.
<a id="sym-830941759893767434"></a>

#### GLX::RGB

```cpp
[Colour](#sym-12411471395882632458) RGB([UInt8](#sym-1020924849394252491) grey);
[Colour](#sym-12411471395882632458) RGB([UInt8](#sym-1020924849394252491) grey, [UInt8](#sym-1020924849394252491) alpha);
[Colour](#sym-12411471395882632458) RGB([UInt8](#sym-1020924849394252491) red, [UInt8](#sym-1020924849394252491) green, [UInt8](#sym-1020924849394252491) blue);
[Colour](#sym-12411471395882632458) RGB([UInt8](#sym-1020924849394252491) red, [UInt8](#sym-1020924849394252491) green, [UInt8](#sym-1020924849394252491) blue, [UInt8](#sym-1020924849394252491) alpha);
```

<a id="sym-15559101598243325194"></a>

#### GLX::RedirectFocus

```cpp
void RedirectFocus([Object](#sym-14361920786363680010)& branch_root, [Object](#sym-14361920786363680010)& object);
```

<a id="sym-11518256371707512074"></a>

#### GLX::Rescale

```cpp
void Rescale(const [ArrayRegion < Point <Float32> >](#sym-2445480886326753995)& points, [Size](#sym-8974655638118434058) scale);
void Rescale(const [ArrayRegion < Tuple < Point <Float32> ,System::Colour> >](#sym-2445480886326753995)& colour_points, [Size](#sym-8974655638118434058) scale);
```

<a id="sym-14510779649853850890"></a>

#### GLX::RetrieveStyleSheet

```cpp
[ConstTRef <StyleSheet>](#sym-532369501975706315) RetrieveStyleSheet(const [WString::View](#sym-5727895666471172811)& path, const [Data::PropertySet](#sym-13039646562789154425)& options_opt);
```

<a id="sym-14933918998927082762"></a>

#### GLX::Rotate

```cpp
void Rotate(const [ArrayRegion < Point <Float32> >](#sym-2445480886326753995)& points, [Point](#sym-1001298644097402122) origin, [Float32](#sym-1452730841175390923) angle_normalised);
void Rotate(const [ArrayRegion < Tuple < Point <Float32> ,System::Colour> >](#sym-2445480886326753995)& colour_points, [Point](#sym-1001298644097402122) origin, [Float32](#sym-1452730841175390923) angle_normalised);
```

<a id="sym-830948468632683786"></a>

#### GLX::Run

```cpp
void Run([Object](#sym-14361920786363680010)& target, [Key32](#sym-974353891938499275) id, [TRef <Animation>](#sym-8974699438245378763) animation);
void Run([Object](#sym-14361920786363680010)& target, [Key32](#sym-974353891938499275) id, [Float32](#sym-1452730841175390923) time, [TRef <Animation>](#sym-8974699438245378763) animation);
void Run([Object](#sym-14361920786363680010)& target, [Key32](#sym-974353891938499275) id, [Float32](#sym-1452730841175390923) time, [InterpolatedAnimation::Easing](#sym-12677384309400002985) easing, [TRef <InterpolatedAnimation>](#sym-8974699438245378763) animation);
```

<a id="sym-9250026225055925514"></a>

#### GLX::ScaleDelta

```cpp
[Point](#sym-1001298644097402122) ScaleDelta(const [Object](#sym-14361920786363680010)& object, [Point](#sym-1001298644097402122) window_coordinates_delta);
```

<a id="sym-15049850890779460874"></a>

#### GLX::Select

```cpp
void Select([Object](#sym-14361920786363680010)& object, bool select);
```

Sets or clears the GLX::kSelectedState ("selected") on the target object
<a id="sym-6899141017635751178"></a>

#### GLX::SelectBranch

```cpp
void SelectBranch([Object](#sym-14361920786363680010)& object, bool select);
```

<a id="sym-5123920649244476682"></a>

#### GLX::SelectChildren

```cpp
void SelectChildren([Object](#sym-14361920786363680010)& object, bool select);
```

<a id="sym-8974635224138876170"></a>

#### GLX::Send

```cpp
bool Send([Object](#sym-14361920786363680010)& src, [Key32](#sym-974353891938499275) id, VARGS... id_value_pairs);
```

Sends a custom event directly to the specified object without propagating it up the object hierarchy.
- src: The object receiving the event.
- id: The Key32 event identifier.
- id_value_pairs: A variadic list of key/value pairs written into the event’s data payload. Keys are identifiers; values may be any supported data type.
- return: boolean indicating whether the event was trapped or not

```cpp
Send(object, "my_event", "value", 1.0f);
```

The call above is equivalent to:
```cpp
auto e = Make<Event>("my_event");
Data::SetFloat(e, "value", 1.0f);
object.ProcessEvent(e);
```

This helper is intended for simple transactional or notification-style events. For advanced cases (preconfigured payloads or reuse), construct an Event explicitly.
<a id="sym-8061677790082204938"></a>

#### GLX::SetBounds

```cpp
void SetBounds([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id, const [Size](#sym-8974655638118434058)& min, const [Size](#sym-8974655638118434058)& max);
```

<a id="sym-8157410691259663626"></a>

#### GLX::SetCanvas

```cpp
void SetCanvas([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id, const [Function <void(GLX::CanvasContext&)>](#sym-5470074934380427979)& callback);
```

<a id="sym-17065274482089886986"></a>

#### GLX::SetClip

```cpp
void SetClip([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id, bool x, bool y);
```

<a id="sym-11451435876106732810"></a>

#### GLX::SetColourCanvas

```cpp
void SetColourCanvas([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id, const [Function <void(GLX::ColourCanvasContext&)>](#sym-5470074934380427979)& callback);
```

<a id="sym-3610858371567158538"></a>

#### GLX::SetEventDelegate

```cpp
void SetEventDelegate([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) delegate_id, const [Function <bool(Object&, Event&)>](#sym-5470074934380427979)& callback);
```

Attaches a delegate to the specified object which forwards all events to the supplied callback.
- object: Target object that emits events.
- delegate_id: a unique id for this handler
- callback: Function called when any event occurs.

SetEventDelegate replaces any previous delegate with the same delegate_id on this object.
The callback should return true if the event is fully handled, to stop propagation. Return false to allow further delegates to receive it.
The following example shows how to intercept and trap mouse clicks. (BindClick is a shorthand for this).
```cpp
GLX::SetEventDelegate(button, "my_delegate", [](GLX::Object & src, GLX::Event & e)
{
	if (e.id == GLX::kMouseDown)
	{
		Log("Button clicked!");
		return true; // stop propagation of mouse down
	}
	return false;	//allow propogation of other events
});
```

<a id="sym-17065738407277331722"></a>

#### GLX::SetFlow

```cpp
void SetFlow([Object](#sym-14361920786363680010)& object, [FlowFlags](#sym-17523243275744544010) flags);
```

<a id="sym-13078888775147422986"></a>

#### GLX::SetGraphicCanvas

```cpp
void SetGraphicCanvas([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id, const [Function <void(GLX::GraphicCanvasContext&)>](#sym-5470074934380427979)& callback);
```

<a id="sym-5762298410296247562"></a>

#### GLX::SetOnStyle

```cpp
void SetOnStyle([Object](#sym-14361920786363680010)& object, const [Function <void(const Style &)>](#sym-5470074934380427979)& callback, [Key32](#sym-974353891938499275) delegate_id_opt);
```

Attaches a delegate which invokes the supplied callback whenever the object's style is changed.
This is a callback-based alternative to overriding the GLX::Object::OnSetStyle method, which can be used to apply sub-styles to child elements when not sub-classing GLX::Object.
- object: target object
- callback: callback to be called every time the object's style is set/changed.
- delegate_id_opt: identifies the delegate instance (so it can be replaced/cleared deterministically).

```cpp
auto container = New<GLX::Object>();
auto button = GLX::AddFloat(container, New<GLX::Button>("Click Me"));
GLX::SetOnStyle(container, [button](const GLX::Style & style)
{
	button->SetStyle(style["button"]);
});
```

<a id="sym-6167080227332785418"></a>

#### GLX::SetOpacity

```cpp
void SetOpacity([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id, [Float32](#sym-1452730841175390923) opacity);
```

<a id="sym-9834429827883566346"></a>

#### GLX::SetState

```cpp
void SetState([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) state, bool value);
```

<a id="sym-17067867804818048266"></a>

#### GLX::SetText

```cpp
void SetText([Object](#sym-14361920786363680010)& object, const [WString](#sym-17787487981880441547)& value, [Key32](#sym-974353891938499275) id_opt);
```

<a id="sym-7415551474236753162"></a>

#### GLX::StartDragDrop

```cpp
void StartDragDrop([TRef <Object>](#sym-8974699438245378763) data, [MouseCursor](#sym-3088564088943510794) dragover, [MouseCursor](#sym-3088564088943510794) block);
```

<a id="sym-8974705575703184650"></a>

#### GLX::Stop

```cpp
void Stop([Object](#sym-14361920786363680010)& target, [Key32](#sym-974353891938499275) id);
```

<a id="sym-14286032820232094986"></a>

#### GLX::ToggleState

```cpp
bool ToggleState([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) state);
```

<a id="sym-1484818945520272650"></a>

#### GLX::TransformPosition

```cpp
[Point](#sym-1001298644097402122) TransformPosition(const [Object](#sym-14361920786363680010)& object, [Point](#sym-1001298644097402122) window_coordinates_position);
```

<a id="sym-10667000377695634698"></a>

#### GLX::Translate

```cpp
void Translate(const [ArrayRegion < Point <Float32> >](#sym-2445480886326753995)& points, [Point](#sym-1001298644097402122) offset);
void Translate(const [ArrayRegion < Tuple < Point <Float32> ,System::Colour> >](#sym-2445480886326753995)& colour_points, [Point](#sym-1001298644097402122) offset);
```

<a id="sym-8092100911750546698"></a>

#### GLX::UnbindEvent

```cpp
void UnbindEvent([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) event_id);
```

<a id="sym-18234720160403522826"></a>

#### GLX::UnsetBounds

```cpp
void UnsetBounds([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id);
```

<a id="sym-18330453061580981514"></a>

#### GLX::UnsetCanvas

```cpp
void UnsetCanvas([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id);
```

<a id="sym-12399408254382343434"></a>

#### GLX::UnsetClip

```cpp
void UnsetClip([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id);
```

<a id="sym-9836085121164346634"></a>

#### GLX::UnsetOpacity

```cpp
void UnsetOpacity([Object](#sym-14361920786363680010)& object, [Key32](#sym-974353891938499275) id);
```

<a id="sym-6841480785334207754"></a>

#### GLX::CanvasContext

<a id="sym-16969424285355574538"></a>

#### GLX::ColourCanvasContext

<a id="sym-4995297399615358218"></a>

#### GLX::GraphicCanvasContext

<a id="sym-14021901793341048074"></a>

#### GLX::Margin

```cpp
[Size](#sym-8974655638118434058) Margin::far;
```

```cpp
[Size](#sym-8974655638118434058) Margin::near;
```

<a id="sym-1009347082097624330"></a>

#### GLX::Range

```cpp
[Float32](#sym-1452730841175390923) Range::length;
```

```cpp
[Float32](#sym-1452730841175390923) Range::start;
```

<a id="sym-13581836222967416074"></a>

#### GLX::AbstractList

Inherits from [Object](#sym-14361920786363680010)
AbstractList is the shared selection and navigation base for list-like widgets.
It handles click, double-click, drag-start, keyboard navigation, Ctrl+A / Ctrl+D selection helpers, and Delete / Backspace remove requests, while derived classes provide item storage and visual updates.
The key events to know are ListSelect, ListLoad, ListStartDrag, and ListRequestRemove. Selection mode can be single, multi, or toggle.
```cpp
void AbstractList::SetSelectionMode([AbstractList::SelectionMode](#sym-832412837035859918) mode);
```

```cpp
[UInt32](#sym-15243775351508400843) AbstractList::GetNumItem();
```

```cpp
void AbstractList::SelectAll();
```

```cpp
void AbstractList::SelectNone();
```

```cpp
bool AbstractList::Select([UInt32](#sym-15243775351508400843) idx, bool multi);
```

```cpp
void AbstractList::Deselect([UInt32](#sym-15243775351508400843) idx);
```

```cpp
bool AbstractList::SelectNext(bool extend);
```

```cpp
bool AbstractList::SelectPrev(bool extend);
```

```cpp
void AbstractList::EnumerateSelection([UInt32](#sym-15243775351508400843) start, [UInt32](#sym-15243775351508400843) range, const [Function <void(UInt idx, UInt n)>](#sym-5470074934380427979)& callback);
```

```cpp
void AbstractList::Reveal([UInt32](#sym-15243775351508400843) idx);
```

<a id="sym-8104147582051255562"></a>

#### GLX::AbstractViewBar

Inherits from [Object](#sym-14361920786363680010)
<a id="sym-9184679921144923402"></a>

#### GLX::AbstractViewPort

Inherits from [Object](#sym-14361920786363680010)
```cpp
void AbstractViewPort::SetContent([TRef <Object>](#sym-8974699438245378763) content, [Key32](#sym-974353891938499275) style_id_opt);
```

```cpp
[TRef <Object>](#sym-8974699438245378763) AbstractViewPort::GetContent();
[ConstTRef <Object>](#sym-532369501975706315) AbstractViewPort::GetContent();
```

```cpp
void AbstractViewPort::InvertScrollAxis(bool invert);
```

```cpp
[TRef <Object>](#sym-8974699438245378763) AbstractViewPort::CreateListener(const [Function <void()>](#sym-5470074934380427979)& callback);
```

```cpp
void AbstractViewPort::SetMinView([Size](#sym-8974655638118434058) size);
```

```cpp
[Size](#sym-8974655638118434058) AbstractViewPort::GetMinView();
```

```cpp
[Size](#sym-8974655638118434058) AbstractViewPort::GetExtent();
```

```cpp
void AbstractViewPort::SetView(const [Rect](#sym-8974479385545508106)& view);
```

```cpp
const [Rect](#sym-8974479385545508106)& AbstractViewPort::GetView();
```

```cpp
[Size](#sym-8974655638118434058) AbstractViewPort::GetPixelsPerUnit();
```

```cpp
void AbstractViewPort::StartScroll(bool yaxis, [Float32](#sym-1452730841175390923) offset);
```

```cpp
void AbstractViewPort::StopScroll(bool yaxis);
```

```cpp
void AbstractViewPort::Reveal(bool yaxis, [Float32](#sym-1452730841175390923) offset, [Float32](#sym-1452730841175390923) range, [Float32](#sym-1452730841175390923) padding, bool animate);
```

```cpp
void AbstractViewPort::EnableAutoScroll([Float32](#sym-1452730841175390923) amount, bool scoped);
```

```cpp
void AbstractViewPort::DisableAutoScroll();
```

```cpp
[ConstTRef <Object>](#sym-532369501975706315) AbstractViewPort::GetBody();
```

```cpp
[TRef <AbstractViewBar>](#sym-8974699438245378763) AbstractViewPort::GetViewBar(bool yaxis);
```

<a id="sym-11673504531626427658"></a>

#### GLX::Animation

Inherits from [Object](#sym-14361920786414140107)
GLX animation covers two closely related systems:
- Animation objects that interpolate values or states over time
- Clocks that execute UI callbacks on the GLX update thread

In many cases the simplest animation is declarative. A stylesheet can define @State variants and a transition time, and code only needs to push or clear the relevant state.

#### State-Based Transitions

For hover, selected, inactive, and similar UI feedback, prefer stylesheet-driven transitions first.
```cpp
Button:
{
	transition: 0.25;

	@State hover:
	{
		bg: Fill(colour: 228);
	};
}
```

This keeps simple UI motion in the style layer rather than in imperative code.

#### Running Explicit Animations

When code needs to decide target values dynamically, create an animation object and run it against a property or state on a target object.
```cpp
auto fade = GLX::CreateColourPropertyAnimation("colour", GLX::kWhite, GLX::kBlack);
GLX::Run(object, "colour", 0.25f, fade);
```

Common helpers in this module include CreateStateAnimation, CreateColourPropertyAnimation, CreateMarginPropertyAnimation, and CreateCallbackAnimation.

#### Clocks

Use clocks when you need procedural updates over time rather than interpolation between two values.
- CreateAnimationClock / AttachAnimationClock for frame-based UI callbacks
- CreatePeriodicClock / AttachPeriodicClock for lower-frequency periodic work
- DetachClock to stop an attached clock

This is commonly used for polling async task status, updating drag visuals, driving custom canvas effects, or other time-based UI behavior that is not well described as a simple property tween.

#### Choosing the Right Approach

- Use stylesheet transition + @State for simple visual feedback
- Use explicit animation objects when code determines the end value or state
- Use clocks for continuous procedural behavior or periodic observation

```cpp
void Animation::SetTime([Float32](#sym-1452730841175390923) time);
```

```cpp
void Animation::SetTarget([Object](#sym-14361920786363680010)& target);
```

```cpp
void Animation::Play();
```

<a id="sym-13415755210635183370"></a>

#### GLX::ContainerAnimation

Inherits from [Animation](#sym-11673504531626427658)
```cpp
void ContainerAnimation::Clear();
```

```cpp
void ContainerAnimation::Add([Animation](#sym-11673504531626427658)& animation);
```

<a id="sym-946331961880839434"></a>

#### GLX::Event

Inherits from [Data::PropertySet](#sym-13039646562789154425)
```cpp
[Key32](#sym-974353891938499275) Event::id;
```

```cpp
[TRef <Event>](#sym-8974699438245378763) Event::Clone();
```

<a id="sym-8972676074806805770"></a>

#### GLX::Form

Inherits from [Object](#sym-14361920786363680010)
Simple non-interactive container widget composed of two sub-objects: header and body.
The header is laid out inline, followed by the body with inline-flex. The default layout direction is vertical (GLX::kFlowY).

### Styling

Applies the header style block to the header object and the body style block to the body object.
```cpp
FormExample:
{
	size: 200,300;

	header:
	{
		size: 32;
		bg_colour: 255,0,0;
	};

	body:
	{
		padding: 8;
		bg_colour: 0,255,0;
	};
}
```

```cpp
const [TRef <Object>](#sym-8974699438245378763) Form::body;
```

```cpp
const [TRef <Label>](#sym-8974699438245378763) Form::header;
```

<a id="sym-17361695219141149962"></a>

#### GLX::InterpolatedAnimation

Inherits from [Animation](#sym-11673504531626427658)
<a id="sym-978729750598092042"></a>

#### GLX::Label

Inherits from [Object](#sym-14361920786363680010)
Label is the lightweight text-holding widget.
It stores a Text property under a chosen property id, so it can be used both for ordinary labels and for small value displays embedded inside larger widgets.
<a id="sym-8973574272727483658"></a>

#### GLX::List

Inherits from [AbstractList](#sym-13581836222967416074)
List is the concrete child-backed implementation of AbstractList.
Selection is reflected directly through each child item's selected state, and it also adds optional drag-reordering through the ListReorder transaction event sequence.
Use it when you already have concrete child objects for each row and want the list to manage selection, reveal, focus, and reorder interaction.
<a id="sym-8973709207715022090"></a>

#### GLX::Menu

Inherits from [Scroller](#sym-16992810420937000202)
The Menu widget provides a standard floating popup menu, typically used for context menus and drop-down menus.
It is built on top of Scroller, so it supports a single scrollable content region and automatically shows scrollbars when needed.
Menus are typically not opened directly. For most cases, use OpenContextMenu which attaches the menu to the window foreground (always-on-top) and triggers a MenuOpen event to allow menu population by the target object and its parents.
Alternatively, use the Popup widget for simple option-enumeration widgets.
```cpp
auto menu = GLX::OpenContextMenu(*this);

GLX::BindClick(menu->AddItem(L"Option 1"), []()
{
	//handle option
});

menu->AddSeparator();

auto sub = menu->AddSubMenu(L"More");
GLX::BindClick(sub->AddItem(L"Option 2"), [](){});
```


#### Styling

Menu inherits Scroller, so implements the same style-schema (body, content, x, y).  Additionally, Menu applies item styles by applying these ids:
- item: applied to items added via AddItem(label).
- separator: applied to separators added via AddSeparator().
- folder: applied to submenu “folder” items added via AddSubMenu(label).

style
```cpp
menu:
{
	size: 256,0;	//min width

	bg: Shadow(width: 16; indent: -8; color: 0,32),Fill(corner: 4);

	content:
	{
		padding: 4;
	};

	item:
	{
		bg: Text(font: MyFont; value: &value; indent: 8,4; color: 0);
	};

	//y: see Scroller
};
```


#### Opening Menus

Typically use OpenContextMenu to create and attach a menu instance.
When the menu is attached, the menu emits Menu::kMenuOpen on src, with the created menu stored as an event property. This allows src and its parents to intercept the open and add items.
To fully support context-menus created on child objects, populate the menu in a seperate handler from creation, e.g:
```cpp
bool SubView::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (e.id == GLX::kMouseDown && (GLX::GetClickFlags(e) & GLX::kClickFlagRmb))
	{
		GLX::OpenContextMenu(*this);
		return true;
	}
	else if (auto menu = GLX::GetMenu(e)) //e.id == Menu::kMenuOpen, reads "menu" property
	{
		GLX::BindClick(menu->AddItem(L"Option 1"), []()
		{
			//handle option
		});

		//return true; //optional: trap to prevent parents adding items
	}

	return GLX::Object::OnEvent(src, e);
}
```

If you don’t need to support interception of menus from child objects, you can open and populate together:
```cpp
bool SubView::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (GLX::IsRightClick(e))	//helper
	{
		auto menu = GLX::OpenContextMenu(*this);

		GLX::BindClick(menu->AddItem(L"Option 1"), []()
		{
			//handle option
		});

		return true;
	}

	return GLX::Object::OnEvent(src, e);
}
```


#### Handling selection

Binding a click handler on every item (BindClick) is often convenient, but for some cases you may prefer a unified handler.
Menu emits Menu::kMenuSelect whenever an item is selected, allowing you to handle all selections in one place. This is useful for large menus, or for simple option/enumeration menus where you only need the selected index.
In this case, use the GetIndex and GetItem helpers to read the index and source item properties from the event:
```cpp
GLX::BindEvent(menu, GLX::kMenuSelect, [](GLX::Object & src, GLX::Event & e)
{
	UInt idx = GLX::GetIndex(e);
	auto item = GLX::GetItem(e);

	//handle selection (idx / item)
	return true;
});
```

```cpp
void Menu::Clear();
```

```cpp
[TRef <Object>](#sym-8974699438245378763) Menu::AddItem(const [WString::View](#sym-5727895666471172811)& label);
[TRef <Object>](#sym-8974699438245378763) Menu::AddItem([TRef <Object>](#sym-8974699438245378763) item);
```

```cpp
[TRef <Object>](#sym-8974699438245378763) Menu::AddSeparator();
[TRef <Object>](#sym-8974699438245378763) Menu::AddSeparator([TRef <Object>](#sym-8974699438245378763) item);
```

```cpp
[TRef <Object>](#sym-8974699438245378763) Menu::AddSubMenu(const [WString::View](#sym-5727895666471172811)& label);
[TRef <Object>](#sym-8974699438245378763) Menu::AddSubMenu([TRef <Object>](#sym-8974699438245378763) item);
```

```cpp
[TRef <Object>](#sym-8974699438245378763) Menu::GetParentItem();
```

```cpp
bool Menu::OpenSubMenu([Object](#sym-14361920786363680010)& item);
```

<a id="sym-986959092620821770"></a>

#### GLX::Multi

Inherits from [ContainerAnimation](#sym-13415755210635183370)
<a id="sym-14361920786363680010"></a>

#### GLX::Object

Inherits from [Data::PropertySet](#sym-13039646562789154425)
```cpp
void Object::SetParent([Object](#sym-14361920786363680010)& child);
```

```cpp
void Object::Clear();
```

```cpp
void Object::InsertBefore([Object](#sym-14361920786363680010)& child);
```

```cpp
void Object::InsertAfter([Object](#sym-14361920786363680010)& child);
```

```cpp
void Object::Detach();
```

```cpp
void Object::SendBottom();
```

```cpp
void Object::SendTop();
```

```cpp
void Object::SetMouseCursor([MouseCursor](#sym-3088564088943510794) mousecursor);
```

```cpp
[MouseCursor](#sym-3088564088943510794) Object::GetMouseCursor();
```

```cpp
void Object::SetStyle(const [Style](#sym-1017425348645717258)& style);
```

```cpp
[ConstTRef <Style>](#sym-532369501975706315) Object::GetStyle();
```

```cpp
void Object::ClearState([Key32](#sym-974353891938499275) state);
```

```cpp
void Object::SetState([Key32](#sym-974353891938499275) state);
```

```cpp
bool Object::CheckState([Key32](#sym-974353891938499275) state);
```

```cpp
void Object::Focus();
```

```cpp
bool Object::ProcessEvent([Object](#sym-14361920786363680010)& src, [Event](#sym-946331961880839434)& e);
```

```cpp
bool Object::Emit([Event](#sym-946331961880839434)& e);
```

```cpp
void Object::Accommodate();
```

```cpp
void Object::Realign();
```

```cpp
void Object::Update();
```

```cpp
void Object::OnAttachWindow();
```

```cpp
void Object::OnDetachWindow();
```

```cpp
void Object::OnClock([Float32](#sym-1452730841175390923) delta);
```

```cpp
void Object::OnUpdate();
```

```cpp
void Object::OnSetStyle(const [Style](#sym-1017425348645717258)& style);
```

```cpp
bool Object::OnEvent([Object](#sym-14361920786363680010)& src, [Event](#sym-946331961880839434)& e);
```

<a id="sym-13030293024079447306"></a>

#### GLX::PlayList

Inherits from [ContainerAnimation](#sym-13415755210635183370)
```cpp
void PlayList::EnableLoop(bool enable);
```

<a id="sym-1001332359590675722"></a>

#### GLX::Popup

Inherits from [Object](#sym-14361920786363680010)
The Popup widget is a lightweight “click-to-open” menu control. When clicked, it opens a floating Menu anchored to the Popup’s screen position (typically displayed underneath the widget if there is space, otherwise positioned to fit).
Popup is built around Menu: it opens a Menu instance and reuses Menu’s item model, styling, and selection events.
See the GLX::Menu documentation for details on populating a menu (items, sub-menus etc) and handling selection events.
```cpp
GLX::BindEvent(m_popup, GLX::Popup::kMenuOpen, [](GLX::Object & src, GLX::Event & e)
{
	auto menu = GLX::GetMenu(e);	//get handle to Menu created by the Popup

	GLX::BindClick(menu->AddItem(L"Option 1"), []()
	{
		//handle option
	});

	return true;
});
```


#### Opening behaviour

When the Popup is clicked, it opens a Menu and then forwards the menu’s Menu::kMenuOpen event to the Popup itself (emitting upward). This allows the Popup, or any of its parents, to intercept the open and populate the menu.
Typical patterns:
- Bind directly on the Popup (recommended for self-contained controls).
- Populate from a parent OnEvent override (recommended when you want parents to extend or override the menu, or share population code).

Example: populate from a parent handler
```cpp
bool ViewImpl::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (src == m_popup && e.id == GLX::Popup::kMenuOpen)
	{
		auto menu = GLX::GetMenu(e);

		menu->AddItem(L"Parent option");

		return true;	//typically trap to prevent ancestors adding items
	}

	return GLX::Object::OnEvent(src, e);
}
```


#### Styling

Popup itself is a widget (Object) that opens a Menu; menu styling is controlled via the Menu style schema. Define a menu sub-style that implements the Menu style schema.
Assuming you have a 'menu' defined previously in your stylesheet, a typical Popup style might look like this:
```cpp
Popup:
{
	color: 0;
	bg: Border(),Text(font: FontID; value: &value; indent: 16,8);	//&value set in code via GLX::SetText

	@Alias menu;   //re-use an existing 'menu' style
}
```

As selecting a menu item will typically lead to a state change, the typical pattern is to display the current value in OnUpdate, for example:
```cpp
void ViewImpl::OnUpdate()
{
	Key32 mode = app->GetMode();                //some state property

	GLX::SetText(m_popup, GetModeString(mode)); //GetModeString is some helper to stringify enum values
}
```

<a id="sym-6607440668686124298"></a>

#### GLX::RangeBar

Inherits from [AbstractViewBar](#sym-8104147582051255562)
Emits a "Transaction" event, see GLX::kTransaction.
<a id="sym-3660065068848088330"></a>

#### GLX::RotarySlider

Inherits from [Object](#sym-14361920786363680010)
RotarySlider is the base numeric drag control used by knob-like widgets such as DragEdit.
It exposes krange and kvalue properties, emits transaction-style updates while dragging, supports keyboard stepping, and provides built-in reset behaviour via right click or double click when those actions are not otherwise handled.
```cpp
void RotarySlider::SetSensitivity([Float32](#sym-1452730841175390923) pixels);
```

```cpp
bool RotarySlider::SetRange([Float32](#sym-1452730841175390923) min, [Float32](#sym-1452730841175390923) max, [Float32](#sym-1452730841175390923) step);
```

```cpp
[Pair <Range,Float32>](#sym-8974152822052584139) RotarySlider::GetRange();
```

```cpp
void RotarySlider::SetDefault([Float32](#sym-1452730841175390923) value);
```

```cpp
[Float32](#sym-1452730841175390923) RotarySlider::GetDefault();
```

```cpp
void RotarySlider::Reset();
```

```cpp
bool RotarySlider::SetValue([Float32](#sym-1452730841175390923) value);
```

```cpp
[Float32](#sym-1452730841175390923) RotarySlider::GetValue();
```

<a id="sym-16992810420937000202"></a>

#### GLX::Scroller

Inherits from [AbstractViewPort](#sym-9184679921144923402)
The Scroller widget provides a scrollable container.
It hosts a single content object and automatically manages x and y scrollbars which appear dynamically when content exceeds the visible region.
Its often used with the List and VirtualList widgets for the content.  It also forms the basis for the standard Menu implementation.
```cpp
auto scroller = New<GLX::Scroller>();
auto content = New<GLX::Object>();

// Set content size (normally comes from content layout)
GLX::SetBounds(content, {}, { 512.0f, 1024.0f });

scroller->SetContent(content);
```

Once the content is assigned, ViewPort automatically calculates the visible region and shows the relevant scrollbars as needed.

#### Styling

Scroller applies the following sub-styles:
- body: background and container styling.
- content: applied to the hosted content object.
- x: horizontal scrollbar.
- y: vertical scrollbar.

Example style block:
```cpp
List:
{
	clip: true;  //alternatively you can set on body (for slightly different clipping behaviour)

	content:
	{
		bg: Tile(stride: 32; axis: y; content: Line(position: bottom; colour: 128; pattern: dashed));
	};

	y:
	{
		size: 32;
		bg:
		Fill(colour: 0,32),
		Bar(range: &range; region: &region; content: Border(colour: 0,64; corner: 4));
	};
};
```

The 'Bar' layer is particularly useful for scrollbar styling.
By binding to 'range' and 'region' (published by the scrollbars), you can draw the visible "trackbar" area that represents the current view region within the total content.

#### Layout Behaviour

By default, Scroller applies kFlowY to its own flow.
The flow direction determines how scrollbars are arranged.
- kFlowY: Y-bar floats over content (right side), X-bar appears inline at the bottom, reducing vertical content space.
- kFlowX: Y-bar appears inline at the right (reducing width), X-bar floats over content at the bottom.

The flow direction of the content (not the ViewPort itself) affects navigation behaviour (mouse wheel, keyboard PageUp/Down, Home/End).

#### Tips & Tricks

- use Reveal to automatically scroll to particular region (see also List and VirtualList Reveal)
- use CreateListener to receive notifications on scrolling
- in your stylesheet, use fg: InnerShadow with the same colour of the content colour as a quick way to blend out content at the edges

<a id="sym-8578895112124335370"></a>

#### GLX::Selector

Inherits from [Object](#sym-14361920786363680010)
Selector is a one-panel-at-a-time content switcher.
Panels are registered up front, then SelectPanel swaps the active content and emits a selection event with both the selected item and index. Depending on style, the content swap can be animated.
EnableContentAutoFit is useful when the selected panel should determine the container size from its real content rather than from style constraints alone.
```cpp
void Selector::EnableContentAutoFit(bool enable);
```

```cpp
void Selector::Clear();
```

```cpp
void Selector::AddPanel([TRef <Object>](#sym-8974699438245378763) item, [Key32](#sym-974353891938499275) style_id_opt);
```

```cpp
void Selector::RemovePanel([UInt32](#sym-15243775351508400843) idx);
```

```cpp
[UInt32](#sym-15243775351508400843) Selector::GetNumPanel();
```

```cpp
[TRef <Object>](#sym-8974699438245378763) Selector::GetPanel([UInt32](#sym-15243775351508400843) idx);
```

```cpp
void Selector::SelectPanel([UInt32](#sym-15243775351508400843) idx);
```

```cpp
[Idx](#sym-830904007181695691) Selector::GetCurrentIndex();
```

<a id="sym-1016746791057589514"></a>

#### GLX::Split

Inherits from [Object](#sym-14361920786363680010)
Split is a thin GLX::Object wrapper around SplitBehaviour for resizable split layouts.
The public API stays intentionally small: you typically set, clear, or query the split size for child items and let the attached behaviour handle the interaction details.
<a id="sym-1017425348645717258"></a>

#### GLX::Style

Inherits from [Data::PropertySet](#sym-13039646562789154425)
```cpp
const [Key32](#sym-974353891938499275) Style::id;
```

```cpp
void Style::SetParent([Style](#sym-1017425348645717258)& child);
```

```cpp
void Style::Clear();
```

```cpp
void Style::InsertBefore([Style](#sym-1017425348645717258)& child);
```

```cpp
void Style::InsertAfter([Style](#sym-1017425348645717258)& child);
```

```cpp
void Style::Detach();
```

```cpp
void Style::Attach([Style](#sym-1017425348645717258)& child);
```

<a id="sym-7196502683237319946"></a>

#### GLX::StyleSheet

Inherits from [Style](#sym-1017425348645717258)
```cpp
const [Key32](#sym-974353891938499275) StyleSheet::path;
```

<a id="sym-1584402070130230538"></a>

#### GLX::TabGroup

Inherits from [Form](#sym-8972676074806805770)
Container widget that extends GLX::Form to provide a tab-based layout.
TabGroup uses a GLX::Selector as its body and populates the form header with tab buttons. Selecting a tab activates the corresponding panel in the underlying selector.
Panels are added with 'AddPanel(label, content, style_id, tab_style_id)'. The 'style_id' is applied to the panel in the selector body, and the 'tab_style_id' is applied to the corresponding tab in the header.
By default, tabs use the header > tab style, and panels use the body > content style.

### Behaviour

Clicking a tab selects the matching panel.
Keyboard navigation is also supported: Tab moves to the next tab and shift+Tab moves to the previous.
The underlying selector can animate panel changes by setting the body > animate property to true.

### Events

TabGroup does not define any custom event IDs of its own.
Selection is driven by the underlying GLX::Selector, and TabGroup forwards the selector's panel-selection notification.
To observe selection changes, either listen on the selector returned by 'GetSelector()', or handle 'GLX::Selector::kSelectPanel' on the TabGroup itself.

### Styling

Extends the Form style schema.
- header: extended with a 'tab' style entry used as the default tab style, plus an align_content property (align_content: [near|far|center|fit];)
- body: applied to the Selector body

The header > align_content property controls how tabs are positioned in the header, via the standard orientation keys (near|far|center|fit)
```cpp
TabGroup
{
	header:
	{
		align_content: center;
	};
};
```

In the simple case, all tabs use the same 'header > tab' style and all panels use 'body > content'.
```cpp
TabGroup:
{
	fg: InnerShadow(width: 0,3,0,0; colour: 0,32);

	header:
	{
		align_content: center;

		bg_colour: 128;
		fg: Line(position: bottom; colour: 200);

		tab:
		{
			transition: 0.25;

			bg:
			[
				Text(indent: 8,4; font: Medium; value: &value; colour: kTextColour)
			];

			@State hover:
			{
				bg:
				[
					Fill(indent: 2,4; colour: 240,128; corner: 4),
					Text(indent: 8,4; font: Medium; value: &value; colour: 0)
				];
			};

			@State selected:
			{
				bg:
				[
					Fill(indent: 2,4; colour: 240; corner: 4),
					Text(indent: 8,4; font: Medium; value: &value; colour: 0)
				];
			};
		};
	};

	body:
	{
		animate: true;		//GLX::Selector property for ease/in animation

		content:
		{
			bg: Fill(colour: 252);
		};
	};
};
```

To style each tab/panel individually, define additional styles under header and pass their ids through the tab_style_id parameter of 'AddPanel(...)'.
```cpp
tabs.AddPanel(L"Overview", overview_panel, "OverviewPanel", "OverviewTab");
tabs.AddPanel(L"Logs", logs_panel, "LogsPanel", "LogsTab");
tabs.AddPanel(L"Settings", settings_panel, "SettingsPanel", "SettingsTab");
```

```cpp
TabGroup:
{
	header:
	{
		align_content: fit;

		@SVG TabIcons:
		{
			path: "icons/tabs.svg";
		};

		OverviewTab:
		{
			size: 36;

			bg:
			[
				Image(source: TabIcons > overview; fit: contain; anchor: top; indent: 8,0; colour: 96),
				Text(font: Medium; value: &value; anchor: bottom; colour: 96)
			];
		};

		LogsTab:
		{
			size: 36;

			bg:
			[
				Image(source: TabIcons > logs; fit: contain; anchor: left; indent: 8,0; colour: 96),
				Text(indent: 30,4; font: Medium; value: &value; colour: 96)
			];
		};

		SettingsTab:
		{
			size: 36;

			bg:
			[
				Image(source: TabIcons > settings; fit: contain; anchor: left; indent: 8,0; colour: 96),
				Text(indent: 30,4; font: Medium; value: &value; colour: 96)
			];
		};
	};

	body:
	{
		animate: true;

		OverviewPanel:
		{
			bg_colour: 200;
		};
	};
};
```

```cpp
void TabGroup::Clear();
```

```cpp
[TRef <Object>](#sym-8974699438245378763) TabGroup::AddPanel(const [WString::View](#sym-5727895666471172811)& label, [TRef <Object>](#sym-8974699438245378763) content, [Key32](#sym-974353891938499275) style_id, [Key32](#sym-974353891938499275) tab_style_id);
```

```cpp
void TabGroup::RemovePanel([UInt32](#sym-15243775351508400843) idx);
```

```cpp
[TRef <Selector>](#sym-8974699438245378763) TabGroup::GetSelector();
[ConstTRef <Selector>](#sym-532369501975706315) TabGroup::GetSelector();
```

<a id="sym-9244439658014803210"></a>

#### GLX::TextArea

Inherits from [Scroller](#sym-16992810420937000202)
TextArea is a scrollable wrapper around TextEditBehaviour.
It owns a content object with a Text property, installs the editing behaviour for that property, and exposes simple ClearText, SetText, and GetText helpers.
The constructor's multi_line flag changes both text flow and scrolling setup, so single-line and multi-line usage are configured differently from the start.
```cpp
void TextArea::ClearText();
```

```cpp
void TextArea::SetText(const [WString](#sym-17787487981880441547)& label);
```

```cpp
[WString::View](#sym-5727895666471172811) TextArea::GetText();
```

<a id="sym-7391239356591211786"></a>

#### GLX::VirtualList

Inherits from [AbstractList](#sym-13581836222967416074)
VirtualList is the large-data counterpart to List.
Instead of owning one child object per row, it materialises only the currently visible items and repopulates them through a callback as the viewport moves. Selection is tracked separately and re-applied when rows come back into view.
This makes it a better fit for long or dynamic datasets, but note that GetItem only returns objects that are currently visible.
```cpp
void VirtualList::SetPopulateCallback(const [Function <void(UInt start, ArrayRegion <Reference<GLX::Object>> items, const GLX::Style & style)>](#sym-5470074934380427979)& callback);
```

```cpp
void VirtualList::ClearItems();
```

```cpp
void VirtualList::SetNumItem([UInt32](#sym-15243775351508400843) n, bool force_refresh);
```

```cpp
void VirtualList::Rebuild();
```

```cpp
[TRef <Object>](#sym-8974699438245378763) VirtualList::GetItem([UInt32](#sym-15243775351508400843) idx);
```

<a id="sym-14450129931335730442"></a>

#### GLX::WindowClient

Inherits from [Object](#sym-14361920786414140107)
<a id="sym-17739790527466931466"></a>

#### GLX::Zoomable

Inherits from [AbstractViewPort](#sym-9184679921144923402)
---


##### [Reflex](#sym-14880872606059205893) > SIMD

<a id="sym-16664858372343377132"></a>

#### SIMD::Boolean

```cpp
kBooleanFalse
kBooleanTrue
```

<a id="sym-12243830513440697580"></a>

#### SIMD::BoolV4

```cpp
using BoolV4 = [TypeV4 <Boolean>](#sym-15320421235171044588);
```

<a id="sym-1452735807989787884"></a>

#### SIMD::FloatV4

```cpp
using FloatV4 = [TypeV4 <Float32>](#sym-15320421235171044588);
```

<a id="sym-965537622992137452"></a>

#### SIMD::IntV4

```cpp
using IntV4 = [TypeV4 <Int32>](#sym-15320421235171044588);
```

<a id="sym-830866282021170412"></a>

#### SIMD::Abs

```cpp
[FloatV4](#sym-1452735807989787884) Abs(const [FloatV4](#sym-1452735807989787884)& value);
[IntV4](#sym-965537622992137452) Abs(const [IntV4](#sym-965537622992137452)& value);
```

<a id="sym-830867918403710188"></a>

#### SIMD::And

```cpp
[BoolV4](#sym-12243830513440697580) And(const [BoolV4](#sym-12243830513440697580)& a, const [BoolV4](#sym-12243830513440697580)& b);
```

<a id="sym-830868008598023404"></a>

#### SIMD::Any

```cpp
bool Any(const [BoolV4](#sym-12243830513440697580)& value);
```

<a id="sym-13508490827818118380"></a>

#### SIMD::ClipNormal

```cpp
[FloatV4](#sym-1452735807989787884) ClipNormal(const [FloatV4](#sym-1452735807989787884)& value);
```

<a id="sym-935139373479406828"></a>

#### SIMD::Count

```cpp
[Int32](#sym-965532656177740491) Count(const [BoolV4](#sym-12243830513440697580)& value);
```

<a id="sym-944995146602687724"></a>

#### SIMD::Empty

```cpp
bool Empty(const [BoolV4](#sym-12243830513440697580)& value);
```

<a id="sym-830888096160066796"></a>

#### SIMD::Exp

```cpp
[FloatV4](#sym-1452735807989787884) Exp(const [FloatV4](#sym-1452735807989787884)& value);
```

<a id="sym-8972563282258341100"></a>

#### SIMD::Exp2

```cpp
[FloatV4](#sym-1452735807989787884) Exp2(const [FloatV4](#sym-1452735807989787884)& value);
```

<a id="sym-8972703281012321516"></a>

#### SIMD::Full

```cpp
bool Full(const [BoolV4](#sym-12243830513440697580)& value);
```

<a id="sym-8391697121869868268"></a>

#### SIMD::GetFlags

```cpp
[Int32](#sym-965532656177740491) GetFlags(const [BoolV4](#sym-12243830513440697580)& value);
```

<a id="sym-5844244906422873324"></a>

#### SIMD::GetFree

```cpp
[UInt32](#sym-15243775351508400843) GetFree(const [BoolV4](#sym-12243830513440697580)& value);
```

<a id="sym-13416385593457814764"></a>

#### SIMD::Invert

```cpp
[FloatV4](#sym-1452735807989787884) Invert(const [FloatV4](#sym-1452735807989787884)& value);
```

<a id="sym-830919522435771628"></a>

#### SIMD::Log

```cpp
[FloatV4](#sym-1452735807989787884) Log(const [FloatV4](#sym-1452735807989787884)& value);
```

<a id="sym-8973600349356600556"></a>

#### SIMD::Log2

```cpp
[FloatV4](#sym-1452735807989787884) Log2(const [FloatV4](#sym-1452735807989787884)& value);
```

<a id="sym-830922288394710252"></a>

#### SIMD::Max

```cpp
[FloatV4](#sym-1452735807989787884) Max(const [FloatV4](#sym-1452735807989787884)& a, const [FloatV4](#sym-1452735807989787884)& b);
[IntV4](#sym-965537622992137452) Max(const [IntV4](#sym-965537622992137452)& a, const [IntV4](#sym-965537622992137452)& b);
```

<a id="sym-830923379316403436"></a>

#### SIMD::Min

```cpp
[FloatV4](#sym-1452735807989787884) Min(const [FloatV4](#sym-1452735807989787884)& a, const [FloatV4](#sym-1452735807989787884)& b);
[IntV4](#sym-965537622992137452) Min(const [IntV4](#sym-965537622992137452)& a, const [IntV4](#sym-965537622992137452)& b);
```

<a id="sym-14091115712889793772"></a>

#### SIMD::Modulo

```cpp
[FloatV4](#sym-1452735807989787884) Modulo(const [FloatV4](#sym-1452735807989787884)& a, const [FloatV4](#sym-1452735807989787884)& b);
```

<a id="sym-830928932709117164"></a>

#### SIMD::Not

```cpp
[BoolV4](#sym-12243830513440697580) Not(const [BoolV4](#sym-12243830513440697580)& a);
```

<a id="sym-25179805120507116"></a>

#### SIMD::Or

```cpp
[BoolV4](#sym-12243830513440697580) Or(const [BoolV4](#sym-12243830513440697580)& a, const [BoolV4](#sym-12243830513440697580)& b);
```

<a id="sym-830938300032789740"></a>

#### SIMD::Pow

```cpp
[FloatV4](#sym-1452735807989787884) Pow(const [FloatV4](#sym-1452735807989787884)& x, const [FloatV4](#sym-1452735807989787884)& y);
```

<a id="sym-8345292607281736940"></a>

#### SIMD::Reciprocal

```cpp
[FloatV4](#sym-1452735807989787884) Reciprocal(const [FloatV4](#sym-1452735807989787884)& value);
```

<a id="sym-16767473181223595244"></a>

#### SIMD::RoundDown

```cpp
[FloatV4](#sym-1452735807989787884) RoundDown(const [FloatV4](#sym-1452735807989787884)& value);
```

<a id="sym-8124602026378702060"></a>

#### SIMD::RoundNearest

```cpp
[FloatV4](#sym-1452735807989787884) RoundNearest(const [FloatV4](#sym-1452735807989787884)& value);
```

<a id="sym-15049850888367156460"></a>

#### SIMD::Select

```cpp
[TypeV4 <TYPE>](#sym-15320421235171044588) Select(const [BoolV4](#sym-12243830513440697580)& mask, const [TypeV4 <TYPE>](#sym-15320421235171044588)& true_value, const [TypeV4 <TYPE>](#sym-15320421235171044588)& false_value);
[TypeV4 <TYPE>](#sym-15320421235171044588) Select(const [BoolV4](#sym-12243830513440697580)& mask, const [TypeV4 <TYPE>](#sym-15320421235171044588)& true_value);
```

<a id="sym-6402223207565798636"></a>

#### SIMD::SelectNot

```cpp
TYPE SelectNot(const [BoolV4](#sym-12243830513440697580)& a, const TYPE& b);
```

<a id="sym-8974652981416340716"></a>

#### SIMD::Sign

```cpp
[FloatV4](#sym-1452735807989787884) Sign(const [FloatV4](#sym-1452735807989787884)& value);
```

<a id="sym-2751346404323011820"></a>

#### SIMD::SquareRoot

```cpp
[FloatV4](#sym-1452735807989787884) SquareRoot(const [FloatV4](#sym-1452735807989787884)& x);
```

<a id="sym-6112104527567564012"></a>

#### SIMD::operator!=

```cpp
[BoolV4](#sym-12243830513440697580) operator!=(const [FloatV4](#sym-1452735807989787884)& a, const [FloatV4](#sym-1452735807989787884)& b);
[BoolV4](#sym-12243830513440697580) operator!=(const [IntV4](#sym-965537622992137452)& a, const [IntV4](#sym-965537622992137452)& b);
[BoolV4](#sym-12243830513440697580) operator!=(const [BoolV4](#sym-12243830513440697580)& a, const [BoolV4](#sym-12243830513440697580)& b);
```

<a id="sym-4657153260484340972"></a>

#### SIMD::operator&

```cpp
[IntV4](#sym-965537622992137452) operator&(const [IntV4](#sym-965537622992137452)& a, const [IntV4](#sym-965537622992137452)& b);
```

<a id="sym-4657153277664210156"></a>

#### SIMD::operator*

```cpp
[FloatV4](#sym-1452735807989787884) operator*(const [FloatV4](#sym-1452735807989787884)& a, const [FloatV4](#sym-1452735807989787884)& b);
[IntV4](#sym-965537622992137452) operator*(const [IntV4](#sym-965537622992137452)& a, const [IntV4](#sym-965537622992137452)& b);
```

<a id="sym-4657153281959177452"></a>

#### SIMD::operator+

```cpp
[FloatV4](#sym-1452735807989787884) operator+(const [FloatV4](#sym-1452735807989787884)& a, const [FloatV4](#sym-1452735807989787884)& b);
[IntV4](#sym-965537622992137452) operator+(const [IntV4](#sym-965537622992137452)& a, const [IntV4](#sym-965537622992137452)& b);
```

<a id="sym-4657153290549112044"></a>

#### SIMD::operator-

```cpp
[FloatV4](#sym-1452735807989787884) operator-(const [FloatV4](#sym-1452735807989787884)& value);
[FloatV4](#sym-1452735807989787884) operator-(const [FloatV4](#sym-1452735807989787884)& a, const [FloatV4](#sym-1452735807989787884)& b);
[IntV4](#sym-965537622992137452) operator-(const [IntV4](#sym-965537622992137452)& a, const [IntV4](#sym-965537622992137452)& b);
```

<a id="sym-4657153299139046636"></a>

#### SIMD::operator/

```cpp
[FloatV4](#sym-1452735807989787884) operator/(const [FloatV4](#sym-1452735807989787884)& a, const [FloatV4](#sym-1452735807989787884)& b);
```

<a id="sym-4657153354973621484"></a>

#### SIMD::operator<

```cpp
[BoolV4](#sym-12243830513440697580) operator<(const [FloatV4](#sym-1452735807989787884)& a, const [FloatV4](#sym-1452735807989787884)& b);
[BoolV4](#sym-12243830513440697580) operator<(const [IntV4](#sym-965537622992137452)& a, const [IntV4](#sym-965537622992137452)& b);
```

<a id="sym-6112108354383424748"></a>

#### SIMD::operator<=

```cpp
[BoolV4](#sym-12243830513440697580) operator<=(const [FloatV4](#sym-1452735807989787884)& a, const [FloatV4](#sym-1452735807989787884)& b);
```

<a id="sym-6112108496117345516"></a>

#### SIMD::operator==

```cpp
[BoolV4](#sym-12243830513440697580) operator==(const [FloatV4](#sym-1452735807989787884)& a, const [FloatV4](#sym-1452735807989787884)& b);
[BoolV4](#sym-12243830513440697580) operator==(const [IntV4](#sym-965537622992137452)& a, const [IntV4](#sym-965537622992137452)& b);
[BoolV4](#sym-12243830513440697580) operator==(const [BoolV4](#sym-12243830513440697580)& a, const [BoolV4](#sym-12243830513440697580)& b);
```

<a id="sym-4657153363563556076"></a>

#### SIMD::operator>

```cpp
[BoolV4](#sym-12243830513440697580) operator>(const [FloatV4](#sym-1452735807989787884)& a, const [FloatV4](#sym-1452735807989787884)& b);
[BoolV4](#sym-12243830513440697580) operator>(const [IntV4](#sym-965537622992137452)& a, const [IntV4](#sym-965537622992137452)& b);
```

<a id="sym-6112108637851266284"></a>

#### SIMD::operator>=

```cpp
[BoolV4](#sym-12243830513440697580) operator>=(const [FloatV4](#sym-1452735807989787884)& a, const [FloatV4](#sym-1452735807989787884)& b);
```

<a id="sym-4657153629851528428"></a>

#### SIMD::operator|

```cpp
[IntV4](#sym-965537622992137452) operator|(const [IntV4](#sym-965537622992137452)& a, const [IntV4](#sym-965537622992137452)& b);
```

<a id="sym-15320421235171044588"></a>

#### template SIMD::TypeV4 <TYPE>

```cpp
TYPE TypeV4::data;
```

```cpp
[TypeV4 <TYPE>](#sym-15320421235171044588)& TypeV4::operator=(const [TypeV4 <TYPE>](#sym-15320421235171044588)& value);
[TypeV4 <TYPE>](#sym-15320421235171044588)& TypeV4::operator=(TYPE value);
```

```cpp
void TypeV4::Set(TYPE value);
void TypeV4::Set(TYPE a, TYPE b, TYPE c, TYPE d);
```

```cpp
[TypeV4 <TYPE>](#sym-15320421235171044588)& TypeV4::operator+=(const [TypeV4 <TYPE>](#sym-15320421235171044588)& value);
```

```cpp
[TypeV4 <TYPE>](#sym-15320421235171044588)& TypeV4::operator-=(const [TypeV4 <TYPE>](#sym-15320421235171044588)& value);
```

```cpp
[TypeV4 <TYPE>](#sym-15320421235171044588)& TypeV4::operator*=(const [TypeV4 <TYPE>](#sym-15320421235171044588)& value);
```

```cpp
[TypeV4 <TYPE>](#sym-15320421235171044588)& TypeV4::operator/=(const [TypeV4 <TYPE>](#sym-15320421235171044588)& value);
```

```cpp
TYPE& TypeV4::operator[]([UInt32](#sym-15243775351508400843) idx);
const TYPE& TypeV4::operator[]([UInt32](#sym-15243775351508400843) idx);
```

```cpp
TYPE* TypeV4::GetData();
const TYPE* TypeV4::GetData();
```

```cpp
TYPE TypeV4::ReadFirst();
```

---


##### [Reflex](#sym-14880872606059205893) > System

<a id="sym-8974154334980923428"></a>

#### System::Path

```cpp
kPathTemp
kPathDesktop
kPathApplicationData
kPathUserData
kPathUserDocuments
```

<a id="sym-11235966723864114280"></a>

#### HttpConnection::Response

```cpp
kResponseAborted
kResponseNoConnection
kResponseOK
kResponsePartialContent
kResponseMovedPermanently
kResponseFound
kResponseBadRequest
kResponseUnauthorized
kResponseForbidden
kResponseNotFound
kResponseInternalServerError
kResponseServiceUnavailable
```

<a id="sym-3088564085798854692"></a>

#### System::MouseCursor

```cpp
kMouseCursorInvisible
kMouseCursorArrow
kMouseCursorWait
kMouseCursorMove
kMouseCursorLeftRight
kMouseCursorTopBottom
kMouseCursorTopLeftBottomRight
kMouseCursorBottomLeftTopRight
kMouseCursorPointer
kMouseCursorDrag
kMouseCursorText
kMouseCursorBlock
kMouseCursorZoom
kNumMouseCursor
```

<a id="sym-9609741836169109540"></a>

#### System::KeyCode

```cpp
kKeyCodeNull
kKeyCodeF1
kKeyCodeF12
kKeyCodeTab
kKeyCodeEnter
kKeyCodeEscape
kKeyCodeSpace
kKeyCodeBackspace
kKeyCodeInsert
kKeyCodeDelete
kKeyCodeHome
kKeyCodeEnd
kKeyCodePageUp
kKeyCodePageDown
kKeyCodeUp
kKeyCodeDown
kKeyCodeLeft
kKeyCodeRight
kKeyCodeNumericDivide
kKeyCodeNumericMultiply
kKeyCodeNumericMinus
kKeyCodeNumericPlus
kKeyCode1
kKeyCode0
kKeyCodeMinus
kKeyCodePlus
kKeyCodeSlash
kKeyCodeA
kKeyCodeZ
kKeyCodeBracketOpen
kKeyCodeBracketClose
kNumKeyCode
```

<a id="sym-16781546976571416612"></a>

#### System::ModifierKeys

```cpp
kModifierKeyShift
kModifierKeyCtrl
kModifierKeyAlt
kModifierKeySystem
```

<a id="sym-7385145563652677668"></a>

#### System::ColourPoint

```cpp
using ColourPoint = [Tuple < Point <Float32> ,Colour>](#sym-1022631093872065227);
```

<a id="sym-18136990298152230948"></a>

#### System::fPoint

```cpp
using fPoint = [Point <Float32>](#sym-1001298644147862219);
```

<a id="sym-1108859096084373540"></a>

#### System::fRect

```cpp
using fRect = [Rect <Float32>](#sym-8974479385595968203);
```

<a id="sym-1109035348657299492"></a>

#### System::fSize

```cpp
using fSize = [Size <Float32>](#sym-8974655638168894155);
```

<a id="sym-12528574736920338468"></a>

#### System::Delete

```cpp
bool Delete(const [WString](#sym-17787487981880441547)& path);
```

<a id="sym-12793038812755386404"></a>

#### System::Exists

```cpp
bool Exists(const [WString](#sym-17787487981880441547)& path);
```

<a id="sym-12080007777000480804"></a>

#### System::GetElapsedTime

```cpp
[Float64](#sym-1452731274967087819) GetElapsedTime();
```

<a id="sym-13432161900956868644"></a>

#### System::GetNumProcessor

```cpp
[UInt32](#sym-15243775351508400843) GetNumProcessor();
```

<a id="sym-13467897282990039076"></a>

#### System::GetOperatingSystemVersion

```cpp
[CString](#sym-17531698610406914763) GetOperatingSystemVersion();
```

<a id="sym-5845711014251847716"></a>

#### System::GetPath

```cpp
[WString](#sym-17787487981880441547) GetPath([Path](#sym-8974154334980923428) path_id);
```

<a id="sym-6411672276618567716"></a>

#### System::GetSystemID

```cpp
[UInt64](#sym-15243775785300097739) GetSystemID();
```

<a id="sym-5846364819943448612"></a>

#### System::GetTime

```cpp
[UInt64](#sym-15243775785300097739) GetTime();
```

<a id="sym-15415839623751000100"></a>

#### System::IsDirectory

```cpp
bool IsDirectory(const [WString](#sym-17787487981880441547)& path);
```

<a id="sym-4686556266557988900"></a>

#### System::MakeDirectory

```cpp
bool MakeDirectory(const [WString](#sym-17787487981880441547)& path);
```

<a id="sym-8974068044792979492"></a>

#### System::Open

```cpp
bool Open(const [WString](#sym-17787487981880441547)& path);
```

<a id="sym-14882056995100287012"></a>

#### System::Rename

```cpp
bool Rename(const [WString](#sym-17787487981880441547)& from, const [WString](#sym-17787487981880441547)& to);
```

<a id="sym-12411471392737976356"></a>

#### System::Colour

```cpp
[Float32](#sym-1452730841175390923) Colour::a;
```

```cpp
[Float32](#sym-1452730841175390923) Colour::b;
```

```cpp
[Float32](#sym-1452730841175390923) Colour::g;
```

```cpp
[Float32](#sym-1452730841175390923) Colour::r;
```

<a id="sym-454116815258760296"></a>

#### HttpConnection::ReceiveDataFn

<a id="sym-9794357931599705192"></a>

#### HttpConnection::ReceiveHeaderFn

<a id="sym-6040881140000219172"></a>

#### System::DirectoryIterator

Inherits from [Object](#sym-14361920786414140107)
<a id="sym-13914065361941159972"></a>

#### System::DiskIterator

Inherits from [Object](#sym-14361920786414140107)
<a id="sym-10482214990098063396"></a>

#### System::DynamicLibrary

Inherits from [Object](#sym-14361920786414140107)
<a id="sym-15071417447436378148"></a>

#### System::FileHandle

Inherits from [Object](#sym-14361920786414140107)
```cpp
bool FileHandle::IsWriteable();
```

```cpp
[UInt64](#sym-15243775785300097739) FileHandle::GetSize();
```

```cpp
void FileHandle::SetPosition([UInt64](#sym-15243775785300097739) position);
```

```cpp
[UInt64](#sym-15243775785300097739) FileHandle::GetPosition();
```

```cpp
[UInt32](#sym-15243775351508400843) FileHandle::Read(void* bytes, [UInt32](#sym-15243775351508400843) buffer_capacity);
```

```cpp
[UInt32](#sym-15243775351508400843) FileHandle::Write(const void* bytes, [UInt32](#sym-15243775351508400843) size);
```

```cpp
bool FileHandle::Truncate();
```

```cpp
bool FileHandle::Flush(bool commit);
```

<a id="sym-5746192168122441764"></a>

#### System::HttpConnection

Inherits from [Object](#sym-14361920786414140107)
```cpp
void HttpConnection::SetTimeout([Float32](#sym-1452730841175390923) connection, [Float32](#sym-1452730841175390923) transfer);
```

```cpp
[HttpConnection::Response](#sym-11235966723864114280) HttpConnection::Request(const [CString::View](#sym-2022350079943473867)& method, const [CString::View](#sym-2022350079943473867)& resource, const [ArrayView < Pair < Array <char> , Array <char> > >](#sym-17111432006044187339)& headers, const [ArrayView <UInt8>](#sym-17111432006044187339)& body, const [HttpConnection::ReceiveHeaderFn](#sym-9794357931599705192)& receive_header, const [HttpConnection::ReceiveDataFn](#sym-454116815258760296)& receive_data);
```

<a id="sym-2589384810356138020"></a>

#### System::Process

Inherits from [Thread](#sym-15234142333668810788)
<a id="sym-10332891952312344612"></a>

#### System::Renderer

Inherits from [Object](#sym-14361920786414140107)
<a id="sym-5663778785163599908"></a>

#### System::Renderer::Canvas

Inherits from [Object](#sym-14361920786414140107)
<a id="sym-8968105079702937636"></a>

#### System::Renderer::Graphic

Inherits from [Object](#sym-14361920786414140107)
<a id="sym-8974771599090769956"></a>

#### System::Task

Inherits from [Object](#sym-14361920786414140107)
<a id="sym-15234142333668810788"></a>

#### System::Thread

Inherits from [Task](#sym-8974771599090769956)
<a id="sym-15742871520433791012"></a>

#### System::Window

Inherits from [Object](#sym-14361920786414140107)
---

