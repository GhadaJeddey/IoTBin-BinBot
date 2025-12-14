# -*- coding: utf-8 -*-

# This sample demonstrates handling intents from an Alexa skill using the Alexa Skills Kit SDK for Python.
# Please visit https://alexa.design/cookbook for additional examples on implementing slots, dialog management,
# session persistence, api calls, and more.
# This sample is built using the handler classes approach in skill builder.
import logging
import paho.mqtt.client as mqtt
import ask_sdk_core.utils as ask_utils

from ask_sdk_core.skill_builder import SkillBuilder
from ask_sdk_core.dispatch_components import AbstractRequestHandler
from ask_sdk_core.dispatch_components import AbstractExceptionHandler
from ask_sdk_core.handler_input import HandlerInput

from ask_sdk_model import Response

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
MQTT_TOPIC1 = "smartbin/lid"
MQTT_TOPIC2 = "smartbin/status"
retrieved_msg = None 

def publish_mqtt_msg(msg):
    try : 
        client = mqtt.Client()
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.publish(MQTT_TOPIC1, msg)
        client.disconnect()
        logger.info(f"message published to {MQTT_TOPIC1} : {msg}")
    except Exception as e:
        logging.error(f"Erro publishing mqtt msg : {e}")

def on_message(client, userdata, message):
    """Callback function to process received MQTT messages."""
    global retrieved_msg
    retrieved_msg = message.payload.decode()
    logger.info(f"Message received on {message.topic}: {retrieved_msg}")

def retrieve_mqtt_msg():
    """
    Connect to the MQTT broker and retrieve the latest message
    from the subscribed topic.
    """
    global retrieved_msg
    retrieved_msg = None  # Reset the retrieved message

    try:
        # Create an MQTT client instance
        client = mqtt.Client()
        client.on_message = on_message

        # Connect to the MQTT broker
        client.connect(MQTT_BROKER, MQTT_PORT, 60)

        # Subscribe to the topic
        client.subscribe(MQTT_TOPIC2)

        # Start the loop to listen for messages
        client.loop_start()

        # Wait for a message (with a timeout of 5 seconds)
        timeout = 5
        start_time = time.time()
        while retrieved_msg is None and (time.time() - start_time) < timeout:
            time.sleep(0.1)  # Small delay to prevent busy-waiting

        # Stop the loop and disconnect
        client.loop_stop()
        client.disconnect() 

        if retrieved_msg:
            logger.info(f"Retrieved message: {retrieved_msg}")
        else:
            logger.warning(f"No message received on topic {MQTT_SUBSCRIBE_TOPIC} within timeout")

        return retrieved_msg

    except Exception as e:
        logger.error(f"Error retrieving MQTT message: {e}")
        return None
        

class LaunchRequestHandler(AbstractRequestHandler):
    """Handler for Skill Launch."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool

        return ask_utils.is_request_type("LaunchRequest")(handler_input)

    def handle(self, handler_input):
        # type: (HandlerInput) -> Response
        speak_output = "Welcome, This application helps you check the storage state of your smart bin"

        return (
            handler_input.response_builder
                .speak(speak_output)
                .ask(speak_output)
                .response
        )


class OpenBinIntentHandler(AbstractRequestHandler):
    """Handler for OpenBinIntent."""
    def can_handle(self, handler_input):
        return ask_utils.is_intent_name("OpenBinIntent")(handler_input)

    def handle(self, handler_input):
        # Send MQTT message to open the bin lid
        publish_mqtt_msg("open")
        speak_output = "The bin lid is now open."

        return (
            handler_input.response_builder
                .speak(speak_output)
                .response
        )
        
class CheckBinStatusIntentHandler(AbstractRequestHandler):
    """Handler for CheckBinStatusIntent."""
    def can_handle(self, handler_input):
        return ask_utils.is_intent_name("CheckBinStatusIntent")(handler_input)

    def handle(self, handler_input):
        # Simulate fetching bin status (replace with actual logic)
        bin_status = "The bin is 50% full."  # Replace with dynamic logic
        speak_output = bin_status

        return (
            handler_input.response_builder
                .speak(speak_output)
                .response
        )
        
class HelloWorldIntentHandler(AbstractRequestHandler):
    """Handler for Hello World Intent."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_intent_name("HelloWorldIntent")(handler_input)

    def handle(self, handler_input):
        # type: (HandlerInput) -> Response
        speak_output = "Hello World!"

        return (
            handler_input.response_builder
                .speak(speak_output)
                # .ask("add a reprompt if you want to keep the session open for the user to respond")
                .response
        )
        
class HelpIntentHandler(AbstractRequestHandler):
    """Handler for Help Intent."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_intent_name("AMAZON.HelpIntent")(handler_input)

    def handle(self, handler_input):
        # type: (HandlerInput) -> Response
        speak_output = "You can say hello to me! How can I help?"

        return (
            handler_input.response_builder
                .speak(speak_output)
                .ask(speak_output)
                .response
        )

class CancelOrStopIntentHandler(AbstractRequestHandler):
    """Single handler for Cancel and Stop Intent."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return (ask_utils.is_intent_name("AMAZON.CancelIntent")(handler_input) or
                ask_utils.is_intent_name("AMAZON.StopIntent")(handler_input))

    def handle(self, handler_input):
        # type: (HandlerInput) -> Response
        speak_output = "Goodbye!"

        return (
            handler_input.response_builder
                .speak(speak_output)
                .response
        )

class FallbackIntentHandler(AbstractRequestHandler):
    """Single handler for Fallback Intent."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_intent_name("AMAZON.FallbackIntent")(handler_input)

    def handle(self, handler_input):
        # type: (HandlerInput) -> Response
        logger.info("In FallbackIntentHandler")
        speech = "Hmm, I'm not sure. You can say Hello or Help. What would you like to do?"
        reprompt = "I didn't catch that. What can I help you with?"

        return handler_input.response_builder.speak(speech).ask(reprompt).response

class SessionEndedRequestHandler(AbstractRequestHandler):
    """Handler for Session End."""
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_request_type("SessionEndedRequest")(handler_input)

    def handle(self, handler_input):
        # type: (HandlerInput) -> Response

        # Any cleanup logic goes here.

        return handler_input.response_builder.response


class IntentReflectorHandler(AbstractRequestHandler):
    """The intent reflector is used for interaction model testing and debugging.
    It will simply repeat the intent the user said. You can create custom handlers
    for your intents by defining them above, then also adding them to the request
    handler chain below.
    """
    def can_handle(self, handler_input):
        # type: (HandlerInput) -> bool
        return ask_utils.is_request_type("IntentRequest")(handler_input)

    def handle(self, handler_input):
        # type: (HandlerInput) -> Response
        intent_name = ask_utils.get_intent_name(handler_input)
        speak_output = "You just triggered " + intent_name + "."

        return (
            handler_input.response_builder
                .speak(speak_output)
                # .ask("add a reprompt if you want to keep the session open for the user to respond")
                .response
        )


class CatchAllExceptionHandler(AbstractExceptionHandler):
    """Generic error handling to capture any syntax or routing errors. If you receive an error
    stating the request handler chain is not found, you have not implemented a handler for
    the intent being invoked or included it in the skill builder below.
    """
    def can_handle(self, handler_input, exception):
        # type: (HandlerInput, Exception) -> bool
        return True

    def handle(self, handler_input, exception):
        # type: (HandlerInput, Exception) -> Response
        logger.error(exception, exc_info=True)

        speak_output = "Sorry, I had trouble doing what you asked. Please try again."

        return (
            handler_input.response_builder
                .speak(speak_output)
                .ask(speak_output)
                .response
        )

# The SkillBuilder object acts as the entry point for your skill, routing all request and response
# payloads to the handlers above. Make sure any new handlers or interceptors you've
# defined are included below. The order matters - they're processed top to bottom.


sb = SkillBuilder()

sb.add_request_handler(LaunchRequestHandler())
sb.add_request_handler(HelloWorldIntentHandler())
sb.add_request_handler(HelpIntentHandler())
sb.add_request_handler(CancelOrStopIntentHandler())
sb.add_request_handler(FallbackIntentHandler())
sb.add_request_handler(SessionEndedRequestHandler())
sb.add_request_handler(IntentReflectorHandler()) # make sure IntentReflectorHandler is last so it doesn't override your custom intent handlers
sb.add_request_handler(OpenBinIntentHandler())
sb.add_request_handler(CheckBinStatusIntentHandler())

sb.add_exception_handler(CatchAllExceptionHandler())

lambda_handler = sb.lambda_handler()




class CheckBinStatusIntentHandler(AbstractRequestHandler):
    """Handler for CheckBinStatusIntent."""
    def can_handle(self, handler_input):
        return ask_utils.is_intent_name("CheckBinStatusIntent")(handler_input)

    def handle(self, handler_input):
        # Retrieve the bin status from the subscribed MQTT topic
        bin_status = retrieve_mqtt_msg()

        if bin_status:
            speak_output = f"The bin is {bin_status}."
        else:
            speak_output = "Sorry, I couldn't retrieve the bin status at this time."

        return (
            handler_input.response_builder
                .speak(speak_output)
                .response
        )

