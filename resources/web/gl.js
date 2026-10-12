// OpenGL ES 3 ABI over a worker-owned WebGL2 context. Handles stay local to
// the rendering worker; Wasm sees GLuint values, never JavaScript objects.
// Keep the exported symbol names aligned with host-imports.txt.
export function createGLImports(memory, canvas, exports) {
    const gl = canvas.getContext("webgl2", { alpha: false, antialias: false, stencil: true });
    if (!gl) throw new Error("WebGL2 is unavailable");
    const objects = [null];
    const add = object => object === null ? 0 : (objects.push(object), objects.length - 1);
    const object = handle => objects[handle] ?? null;
    const bytes = (p, n) => new Uint8Array(memory.buffer, p >>> 0, n >>> 0);
    const ints = (p, n) => new Int32Array(memory.buffer, p >>> 0, n);
    const floats = (p, n) => new Float32Array(memory.buffer, p >>> 0, n);
    const decoder = new TextDecoder();
    const string = p => {
        const view = new Uint8Array(memory.buffer);
        let end = p >>> 0;
        while (view[end]) ++end;
        return decoder.decode(view.slice(p >>> 0, end));
    };
    const writeString = (p, capacity, value, length) => {
        const encoded = new TextEncoder().encode(value || "");
        const size = Math.min(encoded.length, Math.max(0, capacity - 1));
        if (capacity) { bytes(p, capacity).set(encoded.subarray(0, size)); bytes(p + size, 1)[0] = 0; }
        if (length) ints(length, 1)[0] = size;
    };
    const api = {};
    for (const name of ["activeTexture", "blendFunc", "clear", "clearColor", "colorMask", "disable", "enable",
        "drawArrays", "enableVertexAttribArray", "disableVertexAttribArray", "pixelStorei", "scissor", "stencilFunc",
        "stencilOp", "texParameteri", "viewport", "generateMipmap", "renderbufferStorage", "blitFramebuffer",
        "checkFramebufferStatus", "vertexAttribPointer", "getError", "flush"])
        api["gl" + name[0].toUpperCase() + name.slice(1)] = (...args) => gl[name](...args);
    for (const [plural, singular] of [["Buffers", "Buffer"], ["Textures", "Texture"], ["Framebuffers", "Framebuffer"], ["Renderbuffers", "Renderbuffer"]]) {
        api["glGen" + plural] = (n, p) => { for (let i = 0; i < n; ++i) ints(p, n)[i] = add(gl["create" + singular]()); };
        api["glDelete" + plural] = (n, p) => { for (const id of ints(p, n).slice()) { gl["delete" + singular](object(id)); objects[id] = null; } };
        api["glBind" + singular] = (target, id) => gl["bind" + singular](target, object(id));
    }
    api.glCreateProgram = () => add(gl.createProgram());
    api.glCreateShader = type => add(gl.createShader(type));
    for (const name of ["deleteProgram", "deleteShader", "compileShader", "linkProgram", "useProgram"])
        api["gl" + name[0].toUpperCase() + name.slice(1)] = id => {
            gl[name](object(id));
            if (name.startsWith("delete")) objects[id] = null;
        };
    api.glAttachShader = (program, shader) => gl.attachShader(object(program), object(shader));
    api.glShaderSource = (shader, count, pointers, lengths) => {
        let source = "";
        for (let i = 0; i < count; ++i) {
            const p = ints(pointers, count)[i] >>> 0;
            const n = lengths ? ints(lengths, count)[i] : -1;
            source += n < 0 ? string(p) : decoder.decode(bytes(p, n).slice());
        }
        gl.shaderSource(object(shader), source);
    };
    for (const kind of ["Program", "Shader"]) {
        api["glGet" + kind + "iv"] = (id, parameter, p) => {
            const value = parameter === gl.INFO_LOG_LENGTH ? (gl["get" + kind + "InfoLog"](object(id)) || "").length + 1 :
                gl["get" + kind + "Parameter"](object(id), parameter);
            ints(p, 1)[0] = Number(value);
        };
        api["glGet" + kind + "InfoLog"] = (id, capacity, length, p) => writeString(p, capacity, gl["get" + kind + "InfoLog"](object(id)), length);
    }
    api.glBindAttribLocation = (program, index, p) => gl.bindAttribLocation(object(program), index, string(p));
    api.glGetUniformLocation = (program, p) => { const value = gl.getUniformLocation(object(program), string(p)); return value === null ? -1 : add(value); };
    api.glUniform1f = (id, value) => gl.uniform1f(object(id), value);
    api.glUniform1i = (id, value) => gl.uniform1i(object(id), value);
    api.glUniform2fv = (id, count, p) => gl.uniform2fv(object(id), floats(p, count * 2));
    api.glUniform4fv = (id, count, p) => gl.uniform4fv(object(id), floats(p, count * 4));
    api.glUniformMatrix4fv = (id, count, transpose, p) => gl.uniformMatrix4fv(object(id), Boolean(transpose), floats(p, count * 16));
    api.glVertexAttrib4fv = (id, p) => gl.vertexAttrib4fv(id, floats(p, 4));
    api.glBufferData = (target, size, p, usage) => gl.bufferData(target, p ? bytes(p, size) : size, usage);
    api.glBufferSubData = (target, offset, size, p) => gl.bufferSubData(target, offset, bytes(p, size));
    api.glFramebufferTexture2D = (target, attachment, textureTarget, texture, level) => gl.framebufferTexture2D(target, attachment, textureTarget, object(texture), level);
    api.glFramebufferRenderbuffer = (target, attachment, renderbufferTarget, renderbuffer) => gl.framebufferRenderbuffer(target, attachment, renderbufferTarget, object(renderbuffer));
    const pixelData = (p, w, h, format, type) => {
        if (!p) return null;
        if (format !== gl.RGBA || type !== gl.UNSIGNED_BYTE) throw new Error("Reflex WebGL uploads require RGBA8");
        return bytes(p, w * h * 4);
    };
    api.glTexImage2D = (target, level, internal, w, h, border, format, type, p) => gl.texImage2D(target, level, internal, w, h, border, format, type, pixelData(p, w, h, format, type));
    api.glTexSubImage2D = (target, level, x, y, w, h, format, type, p) => gl.texSubImage2D(target, level, x, y, w, h, format, type, pixelData(p, w, h, format, type));
    api.glGetIntegerv = (parameter, p) => {
        const value = gl.getParameter(parameter);
        const values = ArrayBuffer.isView(value) || Array.isArray(value) ? value : [Number(value)];
        ints(p, values.length).set(values);
    };
    const strings = new Map();
    api.glGetString = parameter => {
        if (!strings.has(parameter)) {
            const value = parameter === gl.VERSION ? "3.0 Reflex WebGL2" : parameter === gl.EXTENSIONS ? gl.getSupportedExtensions().join(" ") : String(gl.getParameter(parameter));
            const encoded = new TextEncoder().encode(value + "\0");
            const p = exports().malloc(encoded.length);
            if (!p) throw new Error("Cannot allocate GL string");
            bytes(p, encoded.length).set(encoded);
            strings.set(parameter, p);
        }
        return strings.get(parameter);
    };
    return api;
}
