if (typeof SharedArrayBuffer === 'undefined') {
    const dummyMemory = new WebAssembly.Memory({initial: 0, maximum: 0, shared: true})
    globalThis.SharedArrayBuffer = dummyMemory.buffer.constructor
}
Module = Module || {};
Module.arguments = Module.arguments || ["prog_name"];
for (const i in Module.arguments) {
    Module.arguments[i] = `${Module.arguments[i]}`;
}
Module['print'] = (text) => {
    const element = document.getElementById('output');
    if (text.length > 1) {
        text = Array.prototype.slice.call(text).join('');
    }
    console.log("log: " + text);
    if (element) {
        element.innerText += text + '\n';
    }
};

Module['printErr'] = Module['print'];

const canvas = document.getElementById('canvas');

Module['canvas'] = canvas;
