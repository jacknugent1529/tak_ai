from dataclasses import dataclass
import torch
from tensorboardX import SummaryWriter
import torch.nn.functional as F
from model import TakNet
from dataset import get_tak_dataloader
import click
from torch import Tensor

@dataclass
class TrainState():
    net: torch.nn.Module
    optim: torch.optim.Optimizer
    device: str

def strategy_loss(pred_vals: Tensor, pred_policy: Tensor, idxs: list[tuple], vals: Tensor, ps: list[Tensor]):
    B = len(idxs)

    pred_logits = torch.log_softmax(pred_policy.view((B,-1)), 1).view(pred_policy.shape)

    policy_loss = 0.
    for b in range(B):
        for i in range(len(idxs[b])):
            idx = idxs[b][i]
            pred_logit = pred_logits[b][idx]
            policy_loss -= ps[b][i] * pred_logit # cross entropy
    
    policy_loss /= B;
    
    val_loss = ((vals.flatten() - pred_vals.flatten())**2).sum() / B

    return policy_loss, val_loss

    

def loss_f(data, train_state):
    X, (idxs, p, vals) = data
    X = X.to(train_state.device)
    p = p
    vals = vals.to(train_state.device)

    pred_vals, pred_policy = train_state.net.forward(X)

    return strategy_loss(pred_vals, pred_policy, idxs, vals, p)
    
def train(datafile, logfolder, l, device, epochs, batch_size):
    net = torch.compile(TakNet()).to(device)
    loader = get_tak_dataloader(datafile, batch_size = batch_size)
    writer = SummaryWriter(logfolder)

    optim = torch.optim.AdamW(net.parameters())

    state = TrainState(net, optim, device)

    step = 0
    for epoch in range(epochs):
        for data in loader:                
            policy_loss, val_loss = loss_f(data, state)

            writer.add_scalar("loss/policy", policy_loss, step)
            writer.add_scalar("loss/value", val_loss, step)

            loss = policy_loss + l * val_loss
            
            state.optim.zero_grad()
            loss.backward()
            state.optim.step()
            
            # if step % 50 == 0:
            print(f"step {step}/{len(loader) * epochs}, loss {loss}, policy loss {policy_loss}, value loss {val_loss}")
            step += 1

        torch.save(state.net.state_dict(), f"{logfolder}/model_epoch{epoch}.pt")

@click.command()
@click.option("--datafile", type=str, required=True)
@click.option("--logfolder", type=str, required=True)
@click.option("--device", default="cuda")
@click.option("--epochs", type=int, required = True)
@click.option("--batch-size", default=64)
@click.option("--l", type=float, default = 1)
def main(datafile, logfolder, device, epochs, l, batch_size):
    train(datafile, logfolder, l, device, epochs, batch_size)

if __name__ == "__main__":
    main()


