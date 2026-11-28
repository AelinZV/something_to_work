import unittest
from main import Game, FLOWERS, LETTERS

class TestGame(unittest.TestCase):
    def setUp(self): self.game = Game()
    def to_pet(self):
        self.game.advance(); self.game.advance()
    def to_flowers(self):
        self.to_pet()
        for _ in range(5): self.game.pet()
        self.game.advance()
    def to_letters(self):
        self.to_flowers()
        for key in FLOWERS: self.game.collect(key)
        self.game.advance()
    def to_gift(self):
        self.to_letters()
        for key in LETTERS: self.game.open_letter(key)
        self.game.advance()
    def test_initial(self):
        self.assertEqual(self.game.scene, 'start')
        self.assertFalse(self.game.pet())
    def test_petting_gate(self):
        self.to_pet()
        self.assertFalse(self.game.advance())
        for _ in range(8): self.game.pet()
        self.assertEqual(self.game.love, 5)
        self.assertTrue(self.game.advance())
    def test_unique_flowers(self):
        self.to_flowers()
        self.assertFalse(self.game.collect('bad'))
        self.assertTrue(self.game.collect('rose'))
        self.assertFalse(self.game.collect('rose'))
        self.assertFalse(self.game.advance())
        for key in reversed(tuple(FLOWERS)): self.game.collect(key)
        self.assertEqual(len(self.game.flowers), 5)
        self.assertTrue(self.game.advance())
    def test_letters_any_order(self):
        self.to_letters()
        self.assertFalse(self.game.open_letter('bad'))
        self.game.open_letter('son'); self.game.open_letter('son')
        self.assertEqual(len(self.game.letters), 1)
        self.assertFalse(self.game.advance())
        self.game.open_letter('wish'); self.game.open_letter('hug')
        self.assertTrue(self.game.advance())
    def test_gift_and_final(self):
        self.to_gift()
        self.assertFalse(self.game.advance())
        self.assertTrue(self.game.open_gift())
        self.assertFalse(self.game.open_gift())
        self.assertTrue(self.game.advance())
        self.assertEqual(self.game.scene, 'final')
        self.assertFalse(self.game.advance())
    def test_reset(self):
        self.to_gift(); self.game.open_gift(); self.game.advance()
        self.game.reset()
        self.assertEqual((self.game.scene,self.game.love,self.game.flowers,self.game.letters,self.game.gift_opened),('start',0,[],set(),False))

if __name__ == '__main__': unittest.main()
