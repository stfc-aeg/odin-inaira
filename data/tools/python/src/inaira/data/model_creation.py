import logging


import click
import sys
import yaml
import os

import tensorflow as tf


class ModelCreator():

    def __init__(self, config_file):
        self.config = ModelCreatorConfig(config_file)

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
            tf.keras.layers.experimental.preprocessing.Resizing(
                self.config.image_height, self.config.image_width,
                input_shape=self.config.image_shape
            ),
            tf.keras.layers.experimental.preprocessing.Rescaling(1./255)
        ]
        self.logger.debug("Preprocessing Layers completed")
        
        core_layers = self.conv_2d_pooling_layers(16, self.config.number_colour_layers)
        self.logger.debug("Core Layers Complete")

        dense_layers = [
            tf.keras.layers.Flatten(),
            tf.keras.layers.Dense(128, activation='relu'),
            tf.keras.layers.Dense(self.config.num_classes)
        ]
        self.logger.debug("Dense Layers Completed")
        
        
        self.model = tf.keras.Sequential(
            preprocessing_layers +
            core_layers +
            dense_layers
        )
        self.logger.debug("Model Created")

        if self.config.include_training:
            self.train_model()

    def train_model(self):
        self.logger.debug("Training Model (not currently implemented)")
        pass  # not currently implemented

    def save_model(self):
        self.logger.debug("Saving Model")
        self.model.save(self.config.model_save_location)

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

class ModelCreatorConfig():

    def __init__(self, config_file):

        self.number_colour_layers = 1
        self.input_width = 2000
        self.input_height = 1800
        self.image_width = 2000
        self.image_height = 1800
        self.image_size = (self.input_width, self.input_height)
        self.num_classes = 2
        self.model_save_location = "tf-model"

        self.include_training = False

        if config_file is not None:
            self.parse_file(config_file)

    def parse_file(self, file_name):
        with open(file_name) as config_file:
            config = yaml.safe_load(config_file)
            for(key, value) in config.items():
                setattr(self, key, value)
        self.image_size = (self.input_width, self.input_height)
        self.image_shape = self.image_size + (self.number_colour_layers,)


@click.command()
@click.option('--config', help="The path to the required yml config file.")
def main(config):
    model = ModelCreator(config)
    model.create_model()
    model.save_model()

if __name__ == "__main__":
    main()