import logging

import click
import sys
import tensorflow as tf


class ModelCreator():

    def __init__(self):
        self.color_mode = "grayscale"
        self.number_colour_layers = 1
        self.image_size = (1990, 1990)
        self.image_shape = self.image_size + (self.number_colour_layers,)
        self.num_classes = 2


        self.model = None

        self.logger = logging.getLogger('model_creation')
        ch = logging. StreamHandler(sys.stdout)
        log_format = "%(asctime)s.%(msecs)03d %(levelname)-5s %(name)s - %(message)s"
        formatter = logging.Formatter(fmt=log_format, datefmt="%d/%m/%y %H:%M:%S")
        ch.setFormatter(formatter)
        self.logger.addHandler(ch)

        ch.setLevel("DEBUG")
        self.logger.setLevel("DEBUG")

        self.logger.debug("Model Creation Init")

    def create_model(self):
        self.logger.debug("Creating Model")
        preprocessing_layers = [
            tf.keras.layers.experimental.preprocessing.Rescaling(1./255,
            input_shape=self.image_shape)
        ]
        self.logger.debug("Preprocessing Layers completed")
        
        # core_layers = self.conv_2d_pooling_layers(16, self.number_colour_layers)
        # self.logger.debug("Core Layers Complete")

        dense_layers = [
            tf.keras.layers.Flatten(),
            tf.keras.layers.Dense(128, activation='relu'),
            tf.keras.layers.Dense(self.num_classes)
        ]
        self.logger.debug("Dense Layers Completed")
        
        
        self.model = tf.keras.Sequential(
            preprocessing_layers +
            # core_layers +
            dense_layers
        )
        self.logger.debug("Model Created")

    def train_model(self):
        pass  # not currently implemented

    def save_model(self):
        self.logger.debug("Saving Model")
        self.model.save("tf-model")

    def conv_2d_pooling_layers(self, filters, number_colour_layers):
        return [
            tf.keras.layers.Conv2D(
                filters,
                number_colour_layers,
                padding="same",
                activation='relu'
            ),
            tf.keras.layers.MaxPooling2D()
        ]

# @click.command()
# @click.option('--config', help="The path to the required yml config file.")
def main():
    print("IT RUNS")
    model = ModelCreator()
    model.create_model()
    model.save_model()

if __name__ == "__main__":
    main()