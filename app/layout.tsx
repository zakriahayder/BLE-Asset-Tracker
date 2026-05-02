import type { Metadata } from 'next'
import './globals.css'

export const metadata: Metadata = {
  title: 'Nanotrace Pro',
  description: 'BLE-connected RFID tag inventory management',
  icons: {
    icon: [
      {
        url: '/icon.svg',
        type: 'image/svg+xml',
      },
    ],
  },
}

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode
}>) {
  return (
    <html lang="en" className="bg-background">
      <body className="font-sans antialiased">
        {children}
      </body>
    </html>
  )
}
