const path = require("path");
const fs = require("fs");

const { Configuration, OpenAIApi } = require("openai");
const configuration = new Configuration({
  apiKey: process.env.OPENAI_API_KEY,
});
const openai = new OpenAIApi(configuration);

const model = "text-curie-001";
const pricing = {
  "text-ada-001": 0.0004,
  "text-babbage-001": 0.0005,
  "text-curie-001": 0.002,
  "text-davinci-003": 0.02,
};
let accumulatedCost = parseFloat(fs.readFileSync("cost.txt", "utf-8"));

const fastify = require("fastify")({
  logger: false,
});

fastify.put("/", async function (request, reply) {
  const input = request.body;
  console.log('got message!', input)
  const token = request.headers["auth-token"];
  if (token !== process.env.AUTH_TOKEN) return reply.send("invalid token!");
  if (!input) return reply.send("empty input!");
  if (!/^[a-z]+$/.test(input)) return reply.send("invalid input!");

  let completion
  try {
    completion = await openai.createCompletion({
      model,
      prompt: `Output up to ten valid, complete English corrections for words which are missing their vowels. Only add the letters A, E, I, O, or Y. Do not add any additional consonants or other characters in the corrections.
Input: frnd
Output: [ friend frond ]
Input: mmr
Output: [ immure memoir memory yammer yummier ]
Input: fld
Output: [ afield failed field filed flayed fled flood fluid foaled fold ]
Input: ${input}
Output: [`,
      stop: [' ]'],
    });
  } catch (e) {
    console.error(e);
    return reply.send("error talking to OpenAI!")
  }

  const cost = (pricing[model] * completion.data.usage.total_tokens) / 1000;
  accumulatedCost += cost;
  console.log(`Cost: $${cost}, total so far: $${accumulatedCost}`);
  fs.writeFileSync("cost.txt", accumulatedCost.toFixed(10));

  console.log(completion.data.choices[0]);

  return reply.send(completion.data.choices[0].text);
});

fastify.get("/models", async function (request, reply) {
  return reply.send((await openai.listModels()).data);
})

fastify.get("/cost", function (request, reply) {
  return reply.send(`$${accumulatedCost.toFixed(4)}`);
});

fastify.listen(
  { port: process.env.PORT, host: "0.0.0.0" },
  function (err, address) {
    if (err) {
      fastify.log.error(err);
      process.exit(1);
    }
    console.log(`Your app is listening on ${address}`);
    fastify.log.info(`server listening on ${address}`);
  }
);
