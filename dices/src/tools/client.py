from twisted.internet import reactor, protocol
from enum import Enum
import signal
import re

class State(Enum):
  Idle = 1
  HelloSent = 2
  RollSent = 3
  RollAcked = 4

class RollClient(protocol.Protocol):
  state = State.Idle

  def connectionMade(self):
    self.transport.write(b"hello\n")
    self.state = State.HelloSent

  def dataReceived(self, data):
    if self.state == State.HelloSent:
      self.on_reroll()
    elif self.state == State.RollSent:
      self.handle_roll_result(data)
    else:
      print("Error state")
      self.transport.loseConnection()

  def connectionLost(self, reason):
    print("connection lost")

  def handle_roll_result(self, data):
    data = data.decode().strip()
    # Must be 'won:result={1-6};'
    if len(data) >= 13 and re.match(r"^won\:result=\d+;$", data):
      print("Выпало", data[11:12])
    self.state = State.RollAcked
    reactor.callLater(1, self.on_reroll)

  def on_reroll(self):
    self.transport.write(b"roll\n")
    self.state = State.RollSent
    print("Бросок костей...")


class RollFactory(protocol.ClientFactory):
  protocol = RollClient

  def clientConnectionFailed(self, connector, reason):
    print("Connection failed - goodbye!")
    reactor.stop()

  def clientConnectionLost(self, connector, reason):
    print("Connection lost - goodbye!")
    reactor.stop()

def handle_signal(signal, frame):
  print('SIGINT received, stop')
  reactor.removeAll()
  reactor.stop()

def main():
  signal.signal(signal.SIGINT, handle_signal)

  f = RollFactory()
  reactor.connectTCP("192.168.1.52", 35555, f)
  reactor.run()

if __name__ == "__main__":
  main()
