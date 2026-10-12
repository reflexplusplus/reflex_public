// Decisions about browser defaults stay in this main-thread handler. Reflex
// receives input asynchronously on its application worker, as on Android.
export function attachInput(canvas, worker) {
    const removals = [];
    const pressedButtons = new Set();
    let pendingLeave;
    canvas.tabIndex = 0;
    // Keep the application surface keyboard-focusable without browser focus chrome.
    canvas.style.outline = "none";
    canvas.style.touchAction = "none";
    const modifiers = event => (event.shiftKey ? 1 : 0) | (event.ctrlKey ? 2 : 0) | (event.altKey ? 4 : 0) | (event.metaKey ? 8 : 0);
    const send = (kind, a = 0, b = 0, event = {}) => worker.postMessage({ type: "input", kind, a, b, modifiers: modifiers(event), repeat: Boolean(event.repeat) });
    const listen = (target, name, fn, options) => { target.addEventListener(name, fn, options); removals.push(() => target.removeEventListener(name, fn, options)); };
    const position = event => {
        const rect = canvas.getBoundingClientRect();
        // CSS dimensions can compensate fractional DPR; input stays in Reflex units.
        send(3, (event.clientX - rect.left) * Number(canvas.dataset.logicalWidth || rect.width) / rect.width, (event.clientY - rect.top) * Number(canvas.dataset.logicalHeight || rect.height) / rect.height, event);
    };
    listen(canvas, "pointerenter", event => { pendingLeave = undefined; send(1, 0, 0, event); });
    listen(canvas, "pointerleave", event => {
        // A resize can move the canvas out from under the pointer. Keep GLX's
        // captured pointer attached until mouse-up, including Firefox's leave
        // between pointer-up and the corresponding compatibility mouse-up.
        if (pressedButtons.size) pendingLeave = event;
        else send(2, 0, 0, event);
    });
    listen(canvas, "pointermove", position);
    // Retain movement events when a drag crosses the canvas boundary.
    listen(canvas, "pointerdown", event => canvas.setPointerCapture(event.pointerId));
    listen(canvas, "mousedown", event => { event.preventDefault(); canvas.focus(); position(event); pressedButtons.add(event.button); send(4, event.button === 2 ? 1 : 0, event.detail === 2 ? 1 : 0, event); });
    listen(window, "mouseup", event => {
        if (pressedButtons.delete(event.button)) send(5, event.button === 2 ? 1 : 0, 0, event);
        if (!pressedButtons.size && pendingLeave) {
            send(2, 0, 0, pendingLeave);
            pendingLeave = undefined;
        }
    });
    listen(canvas, "contextmenu", event => event.preventDefault());
    listen(canvas, "wheel", event => { event.preventDefault(); position(event); send(6, -event.deltaX / 100, -event.deltaY / 100, event); }, { passive: false });
    listen(canvas, "focus", event => send(7, 0, 0, event));
    listen(canvas, "blur", event => send(8, 0, 0, event));
    const keys = { Tab:13, Enter:14, Escape:15, Space:16, Backspace:17, Insert:18, Delete:19, Home:20, End:21,
        PageUp:22, PageDown:23, ArrowUp:24, ArrowDown:25, ArrowLeft:26, ArrowRight:27,
        NumpadDivide:28, NumpadMultiply:29, NumpadSubtract:30, NumpadAdd:31, Minus:42, Equal:43, Slash:44, BracketLeft:71, BracketRight:72 };
    for (let i = 1; i <= 12; ++i) keys[`F${i}`] = i;
    for (let i = 1; i <= 9; ++i) keys[`Digit${i}`] = 31 + i;
    keys.Digit0 = 41;
    for (let i = 0; i < 26; ++i) keys[`Key${String.fromCharCode(65 + i)}`] = 45 + i;
    listen(canvas, "keydown", event => {
        event.preventDefault();
        send(9, keys[event.code] || 0, 0, event);
        if (Array.from(event.key).length === 1 && !event.ctrlKey && !event.metaKey) send(11, event.key.codePointAt(0), 0, event);
    });
    listen(canvas, "keyup", event => { event.preventDefault(); send(10, keys[event.code] || 0, 0, event); });
    return () => removals.forEach(remove => remove());
}
