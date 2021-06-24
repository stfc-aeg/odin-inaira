from dataclasses import dataclass, InitVar
import yaml

@dataclass
class FrameProducerConfig:

    log_format = "%(asctime)s.%(msecs)03d %(levelname)-5s %(name)s - %(message)s"
    log_level = "info"
    shared_buffer = "frame_producer"
    shared_mem_size = 1000000
    shared_buffer_size = 1000
    boost_mmap_mode = False
    ready_endpoint = "tcp://127.0.0.1:5001"
    release_endpoint = "tcp://127.0.0.1:5002"
    frames = 10

    config_file: InitVar[str] = None

    def  __getattr__(self, item):
        pass

    def __post_init__(self, config_file):
        if config_file is not None:
            self.parse_file(config_file)

    def parse_file(self, file_name):
        with open(file_name) as config_file:
            config = yaml.safe_load(config_file)

            for (key, value) in config.items():
                setattr(self, key, value)

