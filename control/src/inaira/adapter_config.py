from dataclasses import dataclass

@dataclass
class AdapterConfig:
    endpoints = 'tcp://127.0.0.1:5030'
    default_endpoints = 'tcp://127.0.0.1:530'