from torch.utils.data import Dataset, DataLoader
import json
import torch

WALL_OFFSET = 10

# output tensor channel dimensions:
# - 4 - i
# - 4 - j
# - 6 - flat,wall,left,right,up,down
# - 7 - drop0
# - 8 - drop1
# - 8 - drop2

def encode_move(move):
    def encode_dir(di, dj):
        match (di, dj):
            case (1,0):
                return 2
            case (-1,0):
                return 3
            case (0,1):
                return 4
            case (0,-1):
                return 5
    
    match move['move']:
        case "MOVE":
            return (
                move['i'],
                move['j'],
                encode_dir(move['di'], move['dj']),
                move['drop0'],
                move['drop1'],
                move['drop2'],
            )
        case "FLAT":
            return (
                move['i'],
                move['j'],
                0,
                0, 0, 0,
            )
        case "WALL":
            return (
                move['i'],
                move['j'],
                1,
                0, 0, 0,
            )
        
def encode_board(game):
    assert WALL_OFFSET == 10
    def map_n(n):
        match n:
            case 1:
                return -1
            case 2:
                return 1
            case 11:
                return -2
            case 12:
                return 2
            case 0:
                return 0
            case _:
                assert False, f"got {n}"
    def flip(n):
        if game['turn'] == 2:
            return -n
        else:
            return n
    
    return [[[flip(map_n(n)) for n in col] for col in row] for row in game['board']]

class TakDataset(Dataset):
    def __init__(self, file) -> None:
        super().__init__()

        with open(file) as f:
            self.data = json.load(f)
    
    def __len__(self):
        return len(self.data)

    def __getitem__(self, i):
        state = self.data[i]
        move_idxs = [encode_move(m) for m in state['moves']]
        return (
            torch.Tensor(encode_board(state['game'])),
            (move_idxs, torch.Tensor(state['p']), state['val'])
        )

def tak_collate_fn(batch):
    return (
        torch.stack([X for X,_ in batch]),
        (
            [idxs for _,(idxs,_,_) in batch],
            [p for _,(_,p,_) in batch],
            torch.Tensor([v for _,(_,_,v) in batch]),
        )
    )

def get_tak_dataloader(file, **kwargs):
    ds = TakDataset(file)
    loader = DataLoader(ds, collate_fn=tak_collate_fn, shuffle=True, num_workers=2,**kwargs)
    return loader