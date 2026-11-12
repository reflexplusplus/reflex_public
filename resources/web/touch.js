// Touch contacts retain their identity in both raw and viewport gesture modes.
export function attachTouchInput(canvas, worker, { touchMode, timeOrigin, onViewportGesture })
{
    const contacts = new Map();
    const forwarded = new Set();
    const removals = [];
    const viewport = canvas.parentElement;
    const originalTouchAction = viewport.style.touchAction;
    let requestedMode = touchMode;
    let switchingMode = false;
    let gesturing = false;
    let previousGesture;
    const applyTouchMode = () =>
    {
        touchMode = requestedMode;
        viewport.style.touchAction = touchMode === "viewport" ? "none" : originalTouchAction;
    };
    applyTouchMode();
    const listen = (target, name, callback) =>
    {
        target.addEventListener(name, callback, { passive: false });
        removals.push(() => target.removeEventListener(name, callback));
    };
    const send = (kind, event) =>
    {
        const rect = canvas.getBoundingClientRect();
        const x = (event.clientX - rect.left) * Number(canvas.dataset.logicalWidth || rect.width) / rect.width;
        const y = (event.clientY - rect.top) * Number(canvas.dataset.logicalHeight || rect.height) / rect.height;
        const modifiers = (event.shiftKey ? 1 : 0) | (event.ctrlKey ? 2 : 0) | (event.altKey ? 4 : 0) | (event.metaKey ? 8 : 0);
        const timestamp = (performance.timeOrigin + event.timeStamp - timeOrigin) / 1000;
        worker.postMessage({ type: "touch", kind, id: event.pointerId, x, y, timestamp, modifiers });
    };
    const cancelForwarded = event =>
    {
        for (const id of forwarded)
        {
            const contact = contacts.get(id);
            send(3, {
                pointerId: id, clientX: contact.clientX, clientY: contact.clientY, timeStamp: event.timeStamp,
                shiftKey: event.shiftKey ?? contact.shiftKey, ctrlKey: event.ctrlKey ?? contact.ctrlKey,
                altKey: event.altKey ?? contact.altKey, metaKey: event.metaKey ?? contact.metaKey,
            });
        }
        forwarded.clear();
    };
    const gesture = () =>
    {
        if (contacts.size < 2) return undefined;
        const [a, b] = contacts.values();
        return {
            a: a.pointerId, b: b.pointerId,
            x: (a.clientX + b.clientX) * 0.5,
            y: (a.clientY + b.clientY) * 0.5,
            distance: Math.hypot(a.clientX - b.clientX, a.clientY - b.clientY),
        };
    };
    const finish = (event, cancelled) =>
    {
        if (!contacts.has(event.pointerId)) return;
        if (forwarded.delete(event.pointerId))
        {
            if (!cancelled) send(1, event);
            send(cancelled ? 3 : 2, event);
        }
        contacts.delete(event.pointerId);
        previousGesture = gesture();
        if (!contacts.size)
        {
            gesturing = false;
            switchingMode = false;
            applyTouchMode();
        }
    };
    const cancelAll = event =>
    {
        cancelForwarded(event);
        contacts.clear();
        previousGesture = undefined;
        gesturing = false;
        switchingMode = false;
        applyTouchMode();
    };
    listen(viewport, "pointerdown", event =>
    {
        if (event.pointerType !== "touch") return;
        const onCanvas = event.target === canvas;
        if (!onCanvas && (touchMode !== "viewport" || event.target !== viewport)) return;
        // Suppress compatibility mouse events: touch has its own native bridge.
        event.preventDefault();
        if (onCanvas) canvas.focus({ preventScroll: true });
        event.target.setPointerCapture(event.pointerId);
        contacts.set(event.pointerId, event);
        if (switchingMode) return;
        if (touchMode === "viewport" && contacts.size >= 2)
        {
            cancelForwarded(event);
            gesturing = true;
            previousGesture = gesture();
        }
        else if (onCanvas && !gesturing)
        {
            forwarded.add(event.pointerId);
            send(0, event);
        }
    });
    listen(viewport, "pointermove", event =>
    {
        if (!contacts.has(event.pointerId)) return;
        event.preventDefault();
        contacts.set(event.pointerId, event);
        if (switchingMode) return;
        if (gesturing)
        {
            const current = gesture();
            if (current && previousGesture && current.a === previousGesture.a && current.b === previousGesture.b)
            {
                const scale = previousGesture.distance ? current.distance / previousGesture.distance : 1;
                onViewportGesture({ x: current.x, y: current.y, previousX: previousGesture.x, previousY: previousGesture.y, scale });
            }
            previousGesture = current;
        }
        else if (forwarded.has(event.pointerId)) send(1, event);
    });
    listen(viewport, "pointerup", event => finish(event, false));
    listen(viewport, "pointercancel", event => finish(event, true));
    listen(viewport, "lostpointercapture", event => finish(event, true));
    listen(window, "blur", cancelAll);
    listen(canvas, "blur", cancelAll);
    listen(document, "visibilitychange", event =>
    {
        if (document.visibilityState === "hidden") cancelAll(event);
    });
    return {
        setTouchMode: mode =>
        {
            if (mode !== "raw" && mode !== "viewport") throw new Error('touchMode must be "raw" or "viewport"');
            if (mode === requestedMode) return;
            requestedMode = mode;
            if (contacts.size)
            {
                cancelForwarded({ timeStamp: performance.now() });
                switchingMode = true;
            }
            else applyTouchMode();
        },
        detach: () =>
        {
            cancelAll({ timeStamp: performance.now() });
            for (const remove of removals) remove();
            viewport.style.touchAction = originalTouchAction;
        },
    };
}
