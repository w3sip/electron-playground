const init_obs = require("../bindings.js");
const assert = require("assert");

assert(init_obs, "The expected function is undefined");

function testBasic()
{
    const result =  init_obs("hello");
    assert.strictEqual(result, "Successfully initialized OBS", "Unexpected value returned");
}

assert.doesNotThrow(testBasic, undefined, "testBasic threw an expection");

console.log("Tests passed- everything looks OK!");