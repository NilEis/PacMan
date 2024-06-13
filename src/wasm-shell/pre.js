if (typeof SharedArrayBuffer === 'undefined') {
  const dummyMemory = new WebAssembly.Memory({initial : 0, maximum : 0, shared : true})
  globalThis.SharedArrayBuffer = dummyMemory.buffer.constructor
}

Module = {};
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
