# input tensor channel dimensions
# - 4 - i
# - 4 - j
# - 8 - h

# output tensor channel dimensions:
# - 4 - i
# - 4 - j
# - 6 - left,right,up,down,wall,flat
# - 7 - drop0 - pieces left at start location
# - 8 - drop1 - pieces left at first location
# - 8 - drop2 - pieces left at second location

import torch
from torch import nn
from einops.layers.torch import Rearrange


class ResBlock(nn.Module):
    def __init__(self, channels) -> None:
        super().__init__()
        self.conv1 = nn.Sequential(
            nn.Conv2d(channels, channels, 3, padding='same'),
            nn.BatchNorm2d(channels),
            nn.ReLU()
        )
        self.conv2 = nn.Sequential(
            nn.Conv2d(channels, channels, 3, padding='same'),
            nn.BatchNorm2d(channels),
            nn.ReLU()
        )
    
    def forward(self, x):
        res = x
        x = self.conv1(x)
        x = self.conv2(x)
        x = x + res
        return x


class TakNet(nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.backbone = nn.Sequential(
            Rearrange("b I J C -> b C I J"),
            nn.Conv2d(9, 128, 3, padding='same'),
            ResBlock(128),
            # ResBlock(128)
        )

        # value net
        self.value_net = nn.Sequential(
            # ResBlock(128),
            nn.Conv2d(128, 64, 3), # decrease dimension to 2x2 (no padding)
            Rearrange("b c w h -> b (c w h)"),
            nn.Linear(64 * 2 * 2, 128),
            nn.ReLU(),
            nn.Linear(128,16),
            nn.ReLU(),
            nn.Linear(16, 1),
            nn.Tanh(),
        )

        # policy net
        self.policy_net = nn.Sequential(
            ResBlock(128),
            # ResBlock(128),
            nn.Conv2d(128, 512, 3, padding='same'),
            nn.BatchNorm2d(512),
            nn.LeakyReLU(),
            nn.Conv2d(512, 1024, 3, padding='same'),
            nn.LeakyReLU(),
            nn.Conv2d(1024, 1024, 1, padding='same'),
            nn.LeakyReLU(),
            nn.Conv2d(1024, 2688, 3, padding='same'),
            Rearrange(
                "b (M d0 d1 d2) H W -> b H W M d0 d1 d2", 
                M=6, d0=7, d1=8, d2=8
            ),
        )

    def forward(self, x):
        x = self.backbone(x)

        # value net
        val = self.value_net(x)

        # policy net
        policy = self.policy_net(x)

        return val, policy
